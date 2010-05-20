package Net::PSYC::Irssi;
use strict;
use warnings;
use base 'Net::PSYC::Irssi::Common';

use Net::PSYC::Irssi::Window::Status;
use Net::PSYC::Irssi::Window::Person;
use Net::PSYC::Irssi::Window::Place;
use Net::PSYC::Irssi::Window::Friends;

use Net::PSYC qw(parse_uniform make_uniform same_host dns_lookup get_connection :encrypt Event=Event);
use Event;
use Irssi;
use POSIX;
use Text::Iconv;
use Fcntl qw/F_DUPFD/;

our $VERSION = '0.1';

sub new {
  my ($class) = @_;
  $class = ref $class || $class;
  my $s = {contexts => {}, tags => {}, aliases => {}, raliases => {}, settings => {}};
  bless $s, $class;
  return $s;
}

sub store {
  my ($s) = @_;
  my $h = {};
  $s->store_one($s, $h, $_) for (keys %$s);
  bless $h, ref $s;
  return $h;
}

sub restore {
  my ($s, $upgrade) = @_;
  return unless ref $s->{server} eq 'HASH';
  $s->debug('>> restore:', $upgrade);
  $s->{restore} = 1;
  $s->{upgrade} = $upgrade;

  my $server = Irssi::server_find_tag($s->{server}->{tag});
  if ($server) {
    $s->{server} = $server;
    $s->connect($s->{server}, 1);
  } elsif (!$upgrade) {
    new Irssi::Connect($s->{server})->connect;
  }

  return $s;
}

sub sweep {
  my ($s) = @_;
  return if $s->{disconnected};
  Event::sweep;
  $s->connection_closed unless $s->{conn}->{SOCKET}->connected;
}

sub lookup {
  my ($s, $server, $restore) = @_;
  $s->debug('>> lookup:', $restore);
  $s->{server} = $server;
  my $u = parse_uniform($server->{address});
  my $ip; $ip = dns_lookup($u->{host}) if $u && $u->{host};
  if ($ip) {
    $s->debug($server->{address},'has address', $ip);
    $s->{server_host} = $u->{host};
    Irssi::signal_emit('server connecting', $server, $ip, $restore);
    return 1;
  } else {
    $s->debug('could not get address for', $server->{address});
    Irssi::signal_emit('server connect failed', $server);
    return 0;
  }
}

sub connect {
  my ($s, $server) = @_;
  return $s->connect_failed unless $server;
  $s->debug('>> connect');
  $s->read_settings;

  $s->{server} ||= $server;
  $s->{server_uni} = $s->{uni} = $server->{address};
  $s->{server_uni} =~ s/~(.*)//;
  $s->{nick} ||= $1;

  unless ($s->{uni} && $s->{server_uni} && $s->{nick}) {
    Irssi::print "$s->{uni}: invalid uniform";
    return $s->connect_failed;
  }

  $s->{status} ||= new Net::PSYC::Irssi::Window::Status($s, $s->{uni});
  $s->{contexts}->{''} ||= $s->{status};

  my $fc = $s->{settings}->{friends_channel};
  if ($fc) {
    $s->{friends} ||= new Net::PSYC::Irssi::Window::Friends($s, $fc, $fc);
    $s->{contexts}->{$fc} = $s->{friends};
  } else {
    undef $s->{friends};
  }

  #$s->debug('uni:', $s->{uni});
  #$s->debug('server_uni:', $s->{server_uni});

  $Event::DIED = sub {
    Irssi::print "PSYC: died: @_";
    print STDERR "@_\n";
  };

  Net::PSYC::use_modules('_encrypt') if $server->{use_ssl};
  Net::PSYC::register_uniform('default', $s);
  my $ok;
  $ok = 1 if ref $s->{conn} && $s->{conn}->{OK};
  $s->{conn} = get_connection($s->{uni}, $s->{fd} || $s->{fd0});
  $s->{conn}->{OK} = 1 if $ok;
  $s->{fd0} = $s->{conn}->{SOCKET}->fileno;
  $s->{fd} = fcntl($s->{conn}->{SOCKET},  F_DUPFD, 0) if !$s->{fd} || $s->{fd} == $s->{fd0};

  return $s->connect_failed unless $s->{conn} && $s->{conn}->{SOCKET};
  Net::PSYC::Event::add($s->{conn}->{SOCKET}, 'r', sub { 1; });

  if ($s->{restore}) {
    if ($s->{upgrade}) {
      $s->{server} = $s->server->update({connected => 1});
      Irssi::signal_emit('server connected', $s->server);
    }

    $s->restore_one($s, $_) for (keys %$s);
    delete $s->{restore};
    delete $s->{upgrade};
  } else {
    Net::PSYC::sendmsg($s->{uni}, '_request_link', '', {_password => $s->server->{password}});
  }
  return 1;
}

sub connection_closed {
  my ($s) = @_;
  return if $s->{disconnected};
  if ($s->server->{connected}) {
    $s->disconnected;
  } else {
    $s->connect_failed;
  }
}

sub connect_failed {
  my ($s, $server) = @_;
  $s->{disconnected} = 1;
  $server ||= $s->{server};
  Irssi::signal_emit('server connect failed', $server) if $server;
  return 0;
}

sub connected {
  my ($s, $uni, $name, $mc, $data, $vars) = @_;

  $s->debug('connected to', $s->server->{tag});
  $s->{server} = $s->server->update({connected => 1});
  Irssi::signal_emit('server connected', $s->server);

  $s->{server_uni} = $s->{uni} = $vars->{_source};
  $s->{server_uni} =~ s/~.*//;
  if (exists $vars->{_nick}) {
    $s->{nick} = $vars->{_nick};
  }
  Net::PSYC::get_connection($s->{uni})->TRUST(6);
  $s->sendmsg($s->{uni}, '_request_execute', 'set echo on');

  $s->command($s->{uni}, 'alias', undef, {noprint => 1});
  $s->command($s->{uni}, 'set', 'nick');
  $s->command($s->{uni}, 'set', 'speakaction');
  $s->sendmsg($s->{uni}, '_request_do_list_places');
  $s->sendmsg($s->{uni}, '_request_do_list_peers');
  $s->sendmsg($s->{uni}, '_request_do_presence', '', {_degree_availability => 7, _degree_automation => 0});
}

sub disconnect {
  my ($s, $msg) = @_;
  $s->debug('>> disconnect:', $s->server->{tag});
  #$s->{nodisconnect} = $nodisconnect;

  $s->sendmsg($s->{uni}, '_request_do_presence', '', {_degree_availability => 1, _degree_automation => 0, _description_presence => $msg});
  $s->sendmsg($s->{uni}, '_request_do_quit');

  #return if Event::loop(1);
  $s->disconnected;
}

sub disconnected {
  my ($s) = @_;
  $s->debug('>> disconnected:', $s->server->{tag});
  return if $s->{disconnected};
  $s->{disconnected} = 1;
  Event::sweep;
  Net::PSYC::unregister_uniform();
  Net::PSYC::Event::remove();
  Net::PSYC::shutdown($s->{conn});
  POSIX::close($s->{fd});

  $s->server->disconnect if $s->server->{connected} && !$s->{nodisconnect};
  #Event::unloop(1);
}

sub read_settings {
  my ($s) = @_;
  my $ss = $s->{settings};
  $ss->{friends_channel} = Irssi::settings_get_str('psyc_friends_channel');
  $ss->{friends_auto_update} = Irssi::settings_get_bool('psyc_friends_auto_update');
  $ss->{display_speakaction} = Irssi::settings_get_bool('psyc_display_speakaction');
  $s->set_debug(Irssi::settings_get_bool('psyc_debug'));

  #my $recode = Irssi::settings_get_bool('recode');
  #my $term = Irssi::settings_get_str('term_charset');
  #my $out = Irssi::settings_get_str('recode_out_default_charset');

  #delete $s->{recode} if !$recode || $term ne ($ss->{term_charset}||'');
  #delete $s->{recode_out} if !$recode || $term ne ($ss->{term_charset}||'') || $out ne ($ss->{recode_out_default_charset}||'');

  #if ($recode) {
  #  $s->{recode} = new Text::Iconv('UTF8', $term) unless $s->{recode};
  #  if ($out && $out ne $term) {
  #    $s->{recode_out} = new Text::Iconv($term, $out) unless $s->{recode_out};
  #  }
  #}

  #$ss->{recode} = $recode;
  #$ss->{term_charset} = $term;
  #$ss->{recode_out_default_charset} = $out;
}

sub get_context {
  my ($s, $vars, $auto, $witem) = @_;
  my $uni = $s->get_uni($vars, 1);
  my $obj = $s->{contexts}->{$uni};
  $s->debug('>> get_context:', $uni, $obj, $auto);
  #$s->debug('>>> contexts:', join ', ', keys %{$s->{contexts}});
  return $obj if $obj;
  return if $auto && $auto < 0;

  my $name = $s->get_name($vars, $uni);
  if ($obj = $s->new_window($uni, $name, $auto, $witem)) {
    $s->{contexts}->{$uni} = $obj;
    $obj->init;
    return $obj;
  }
}

sub verify_enter {
  my ($s, $vars, $tag) = @_;
  my $uni = $s->get_uni($vars, 1);
  return 1 if exists $s->{contexts}->{$uni};

  unless ($tag || Net::PSYC::get_connection($s->{uni}) eq $vars->{_INTERNAL_origin}) {
    $s->print("$uni is trying to join us into a room without a proper _tag");
    return 1;
  }

=begin comment
  my $u = parse_uniform($uni);
  if (ref $u && $u->{'object'} =~ /^\@/) {
    my $silent = (exists $vars->{_control}
                  && $vars->{_control} eq '_silent') ? 1 : 0;
    my $name;
    if (same_host($u->{host}, $SERVER_HOST)) {
	    $name = substr($u->{object}, 1);
    } else {
	    $name = $addr;
    }
    $obj = &{$new}( $addr, $name, $silent );
    register_context($addr, $obj);
    return 1;
  }
  return 0;
=end comment
=cut
}

sub msg {
  my ($s, $source, $mc, $data, $vars) = @_;
  $s->debug('>> msg:', $s->server->{tag}, $mc, $source, $data);
  #$s->dumper($vars);
  return if $s->{disconnected};

  #workaround for privmsg bug
  if ($mc =~ /^_message_private/ && $vars->{_source} eq $vars->{_source_relay} && $vars->{_source} eq $s->{uni}) {
    $vars->{_source_relay} =~ s/~.*/~$vars->{_nick}/;
  }

  my $uni = $s->get_source($vars);
  my $name = $s->get_name($vars, $uni);
  my $t = $vars->{_tag_relay} || $vars->{_tag_reply};
  my $tag; $tag = delete $s->{tags}->{$t} if $t;
  #$s->dumper($tag);
  #$s->dumper($vars);
  my @p = ($uni, $name, $mc, $data, $vars, $tag);
  my $obj; $obj = $s->{status} if !$vars->{_context} && $uni eq $s->{uni};

  $_ = $mc;
 SWITCH: {
    /^_notice_circuit_established$/ && do {
      if ($vars->{_source} eq $s->{server_uni}) {
        $s->{unl} = $vars->{_target_peer};
        #Net::PSYC::register_uniform($s->{unl}, $s);
      }
    };
    /^_status_circuit$/ && do {
      #Net::PSYC::unregister_uniform('default');
    };
    /^_notice_link$/ && do {
      my @user = parse_uniform($source);
      my @uni = parse_uniform($s->{uni});
      if (same_host($user[1], $uni[1]) && lc($user[4]) eq lc($uni[4])) {
        $s->connected(@p);
      } else {
        $s->print("Got a _notice_link from an uni ($source) we did not try to link to.");
      }
      last SWITCH;
    };
    /^(_notice_link_removed|_notice_unlink|_echo_logoff|_error_invalid_password)$/ && do {
      $obj = $s->get_context($vars);
      $obj->msg(@p) if $obj;
      $s->disconnected;
      last SWITCH;
    };
    /^_request_authentication/ && do {
      if ($vars->{_location} =~ m,^psyc://(.+):,) {
        if (same_host($1, $s->{server_host})) {
          $s->sendmsg($uni, '_notice_authentication', "",
                               {_trust_result => 3, _location => $vars->{_location}, _nick => $vars->{_nick}});
        } else {
          $s->sendmsg($uni, '_error_invalid_authentication');
        }
      }
      last SWITCH;
    };
    /^_query_password$/ && do {
      $s->sendmsg($s->{uni}, '_set_password', '', {_password => $s->server->{password}});
      last SWITCH;
    };
    /^_(echo|info)_set/ && do {
      last SWITCH unless $source eq $s->{uni};
      my $key = $vars->{_key} || $vars->{_key_set};
      if ($key eq 'name') {
        $s->{nick} = $vars->{_value} || lc $s->{nick};
        my $snick = $s->server->{nick};
        if ($snick ne $s->{nick}) {
          $s->{server} = $s->server->update({nick => $s->{nick}});
          Irssi::signal_emit('server nick changed', $s->server);
          $s->nick_change($snick, $s->{nick}, $s->{uni});
        }
      } elsif ($key eq 'speakaction') {
        $s->{action} = $vars->{_value};
      }
    };
    /^_info_description$/ && do {
      if ($source eq $s->{uni}) {
        $$s->{action} = $vars->{_action};
      }
    };
    /^_list_alias$/ && do {
      my $long = lc $s->name2uni($vars->{_long});
      my $short = $vars->{_short};
      $s->{aliases}->{lc $long} = $short;
      $s->{raliases}->{lc $short} = $long;
    };
    /^_echo_alias_added$/ && do {
      my $long = lc $s->name2uni($vars->{_address});
      my $short = $vars->{_alias};
      $s->{aliases}->{lc $long} = $short;
      $s->{raliases}->{lc $short} = $long;
      $s->nick_change($long, $short, $long);
    };
    /^_echo_alias_removed$/ && do {
      my $long = lc $s->name2uni($vars->{_address});
      my $short = $vars->{_alias};
      delete $s->{aliases}->{lc $long};
      delete $s->{raliases}->{lc $short};
      $s->nick_change($short, $long, $long);
    };
    /^_list_places_entered$/ && do {
      $obj = $s->get_context($vars->{_location_place}, 1);
      $s->{status}->msg(@p);
      last SWITCH;
    };
    /^_echo_place_enter_automatic_subscription/ && do {
      $obj = $s->get_context($vars, 1);
      last SWITCH;
    };
    /^_echo_place_enter/ && do {
      unless (exists $s->{contexts}->{$uni} || $tag || Net::PSYC::get_connection($s->{uni}) eq $vars->{_INTERNAL_origin}) {
        $s->print("$uni is trying to join us into a room without a proper _tag");
        #last SWITCH;
      }
    };
    /^_echo_place_leave/ && do {
      $obj = $s->get_context($vars, -1);
      last SWITCH unless $obj;
      if ($obj->{type} eq 'place') {
        $obj->msg(@p);
        $obj->destroy(@p);
        delete $s->{contexts}->{$obj->{uni}};
      }
      last SWITCH;
    };
    /^_notice_place_leave(_invalid|_subscribe)?/ && do {
      $s->debug('!!!! leave',$source,$uni,$s->{uni});
      my $invalid = $1 && $1 eq '_invalid';
      my $subscribe = $1 && $1 eq '_subscribe';
      if ($source eq $s->{uni} || $uni eq $s->{uni} || $invalid) {
        $vars->{_context} = $vars->{_source} if $subscribe;
        $obj = $s->get_context($vars, -1);
        $s->debug('!!!! leave: obj:',$obj);
        if ($invalid && !$obj) {
          $s->{status}->msg(@p);
        }
        last SWITCH unless $obj;
        $obj->{noleave} = 1;
        $obj->msg(@p);
        $obj->destroy(@p);
        delete $s->{contexts}->{$obj->{uni}};
        last SWITCH;
      }
    };
    /^_message(_echo)?/ && do {
      if ($uni eq $s->{uni} && $vars->{_nick_target}) {
        $obj = $s->get_context($s->get_name($vars, $vars->{_nick_target}));
      } else {
        $obj = $s->get_context($vars);
      }
      $obj->msg(@p) if $obj;
      last SWITCH;
    };
    /^_notice_presence |
     ^_status_person |
     ^_list_acquaintance |
     ^_list_friends |
     ^_request_friendship | ^_notice_friendship | ^_echo_friendship_removed |
     _friendship
    /x && do {
      if ($s->{friends}) {
        $s->{friends}->msg(@p);
        last SWITCH;
      }
    };
    /^(_error_necessary_membership|_failure_redirect)/ && do {
      $s->{status}->msg(@p);
      last SWITCH;
    };

    $obj = $s->get_context($vars) unless $obj;
    $obj->msg($uni, $name, $mc, $data, $vars, $tag) if $obj;
  }
}

sub new_window {
  my ($s, $uni, $name, $auto, $witem) = @_;
  $s->debug('** new window:', $uni, $name, $auto);
  if (my $u = parse_uniform($uni)) {
    my $obj;
    if ($s->is_place($u->{object})) {
      $obj = new Net::PSYC::Irssi::Window::Place($s, $uni, $name, $auto, $witem);
    } elsif (($u->{scheme} eq 'psyc' && $s->is_person($u->{object})) || ($u->{scheme} eq 'xmpp' && $u->{user})) {
      $obj = new Net::PSYC::Irssi::Window::Person($s, $uni, $name, $auto, $witem);
    } else {
      $obj = $s->{status};
    }
    return $obj;
  }
}

sub sendmsg {
  my ($s, $target, $mc, $data, $vars, $MMPvars, $params) = @_;
  #$s->debug('sendmsg:', $target, $mc, $data, $vars, $MMPvars);
  return 0 unless $mc;
  $target = $s->{uni} if !$target || $target eq $s->{settings}->{friends_channel};
  $data = '' unless defined $data;
  $data = Irssi::recode_out($s->server, $data, $target);
  #$data = $s->{recode_out}->convert($data) || $data if $s->{recode_out};

  $target = $s->name2uni($target);
  $s->debug('sendmsg:', $target, $mc, $data, $vars, $MMPvars);

  $MMPvars ||= {};
  $MMPvars->{_source_identification} ||= $s->{uni};

  $vars ||= {};
  if ($mc =~ /^_message/) {
    $vars->{_nick} ||= $s->server->{nick};
  }

  if ($target =~ /^xmpp:/) {
    register_route($target, get_connection($s->{uni}));
  }

  if (exists $vars->{_tag}) {
    delete $vars->{_tag} unless $vars->{_tag};
  } else {
    $vars->{_tag} = unpack("H*",pack("F", rand(10)));
  }
  $s->{tags}->{$vars->{_tag}} = {target => $target, mc => $mc, data => $data, vars => $vars, MMPvars => $MMPvars, params => $params || {}} if $vars->{_tag};

  #$s->dumper($vars);
  #$s->dumper($MMPvars);
  Net::PSYC::sendmsg($target, $mc, $data, $vars, $MMPvars);
}

sub execute {
  my ($s, $target, $command, $data, $params) = @_;
  $command .= " $data" if $data;
  my $vars = {};
  $vars->{_focus} = $target if $target;
  $s->sendmsg($s->{uni}, '_request_execute', $command, $vars, {}, $params);
}

sub command {
  my ($s, $target, $command, $data, $params) = @_;
  $s->debug('** command:', $target, $command, $data, '**');

  my @cmd;
  $_ = $command;
 SWITCH: {
    /^enter$/ && do {
      @cmd = ($s->{uni}, '_request_do_enter', '', {_group => $s->place2uni($data)});
      last SWITCH;
    };
    /^leave$/ && do {
      @cmd = ($s->{uni}, '_request_do_leave', '', {_group => $s->place2uni($data)});
      last SWITCH;
    };
    /^friends$/ && do {
      if (my $f = $s->{friends}) {
        $f->{sb_away_old} = $f->{sb_away};
        $f->{sb_here_old} = $f->{sb_here};
        $f->{sb_here} = $f->{sb_away} = [];
      }
      last SWITCH;
    }
  };
  if (@cmd) {
    $s->sendmsg(@cmd, $params);
  } else {
    $s->execute($target, $command, $data, $params);
  }
  return 1;
}

sub talk {
  my ($s, $target, $type, $data, $action) = @_;
  $s->debug('>> talk:', $target, $type, $data, $action);
  return if $target eq $s->{settings}->{friends_channel};

  my $obj = $s->get_context($target);
  $s->debug('>>> obj:', $obj);
  $obj->talk($data, $action) if $obj;

  my $vars = {};
  $vars->{_action} = $action if defined $action;
  $vars->{_focus} = $target;
  $type = $type ? 'private' : 'public';

  $s->sendmsg($s->{uni}, "_request_do_message_$type", $data, $vars);
}

sub away {
  my ($s, $data) = @_;
  $s->{server} = $s->server->update({usermode_away => $data ? 1 : 0, away_reason => $data});
  Irssi::signal_emit('away mode changed', $s->server);
  $s->command($s->{uni}, $data ? 'away' : 'here', $data);
}

sub action {
  my ($s, $target, $action);
  $s->sendmsg($target, "_request_do_action_$action");
}

sub enter {
  my ($s, $target, $force) = @_;
  return 0 unless $target;

  $s->command(undef, 'enter', $target);

  if ($force) {
    my $uni = $target;
    $uni = '@'.$target unless $s->is_place($target);
    $s->get_context($uni);
  }
  return 1;
}

sub leave {
  my ($s, $target) = @_;
  return 0 unless $target;
  $s->debug('>> leave:', $target);

  if (my $place = $s->{contexts}->{$target}) {
    $place->destroy;
    #delete $s->{contexts}->{$target};
  } else {
    $s->command(undef, 'leave', $target);
  }
  return 1;
}

sub query_created {
  my ($s, $witem, $auto) = @_;
  return if $s->{restore};
  my $uni = $s->name2uni($witem->{name});
  my $name = $s->get_name($uni);
  my $obj = $s->get_context($uni, $auto, $witem);

  $witem = $witem->update({name => $uni, visible_name => $name, address => $uni});
  Irssi::signal_emit("window item name changed", $witem);
  Irssi::signal_emit("query address changed", $witem);
}

sub channel_created {
  my ($s, $witem, $auto) = @_;
  return if $witem->{name} eq $s->{settings}->{friends_channel} || $s->{restore};
  my $uni = $s->place2uni($witem->{name});
  my $name = $s->get_name($uni);
  my $obj = $s->get_context($uni, $auto, $witem);
  #$s->enter($uni);

  $witem = $witem->update({name => $uni, visible_name => $name});
  Irssi::signal_emit("window item name changed", $witem);
}

sub witem_created {
  my ($s, $witem) = @_;
  return if !$witem || $s->{restore};
  my $type = $witem->{type};
  my $uni = $s->name2uni($witem->{name});
  $s->debug('>> new witem:', $type, $uni);

  my $obj = $s->get_context($uni);
  #my $obj = $s->{contexts}->{$uni};
  return unless $obj;
  $obj->witem_created($witem);

  if ($obj->{type} eq 'place') {
    $s->sendmsg($uni, '_request_status');
    #$s->sendmsg($uni, '_request_members');
  }
  return 1;
}

sub witem_removed {
  my ($s, $witem) = @_;
  return 0 unless $witem;

  my $uni = $witem->{name};
  if (my $obj = $s->{contexts}->{$uni}) {
    $obj->witem_removed;
    delete $s->{contexts}->{$uni};
  }
  return 1;
}

sub nick_change {
  my ($s, $old, $new, $uni) = @_;
  $s->debug('>> nick change:', $old, $new, $uni);
  for my $k (keys %{$s->{contexts}}) {
    my $obj = $s->{contexts}->{$k};
    $s->debug('>>>', $k, $obj);
    $obj->nick_change($old, $new, $uni);
  }
  if (lc $new eq lc $s->{nick}) {
    Irssi::signal_emit('message own_nick', $s->server, $new, $old, $uni);
  } else {
    Irssi::signal_emit('message nick', $s->server, $new, $old, $uni);
  }
}

sub nick {
  my ($s, $target, $nick) = @_;
  if (lc $nick eq lc $s->{nick}) {
    $s->command($s->{uni}, 'set nick', $nick);
  } else {
    $s->command($target, 'nick', $nick);
  }
}

sub window_changed_to {
  my ($s, $name) = @_;
  return unless $name eq $s->{settings}->{friends_channel};
  $s->{friends}->window_changed_to;
  $s->command($s->{uni}, 'friends') if $s->{settings}->{friends_auto_update};
}

sub window_changed_away {
  my ($s, $name) = @_;
  return unless $name eq $s->{settings}->{friends_channel};
  $s->{friends}->window_changed_away;
}

sub sb_friends_here {
  my ($s) = @_;
  return unless $s->{friends};
  return $s->{friends}->{sb_here};
}

sub sb_friends_away {
  my ($s) = @_;
  return unless $s->{friends};
  return $s->{friends}->{sb_away};
}

sub set_terminal_size {
  my ($s, $w, $h) = @_;
  $s->{settings}->{terminal}->{width} = $w;
  $s->{settings}->{terminal}->{height} = $h;
}

sub list {
  my ($s) = @_;
}

sub server {
  shift->{server};
}

sub set_debug {
  my ($s, $d) = @_;
  $s->{settings}->{debug} = $d ? 1 : 0;
  $d = $d ? 2 : 0;
  Net::PSYC::setDEBUG($d) if Net::PSYC::DEBUG ne $d;
}

sub LOAD {}
sub UNLOAD {}

1;
