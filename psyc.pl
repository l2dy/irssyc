use strict;
use warnings;
use lib "$ENV{HOME}/.irssi/lib";

use Irssi;
use Irssi::TextUI;
use Net::PSYC qw/parse_uniform/;
use Net::PSYC::Irssi;

use Storable;
use Data::Dumper;
use Term::ReadKey;
use IO::Socket;
use Module::Reload;
use Getopt::Long;

Getopt::Long::Configure('bundling_override');

our $VERSION = '0.1';
our %IRSSI = (
              authors     => 'Gabor Adam TOTH',
              name        => 'psyc',
              description => 'psyc protocol implementation for irssi',
              license     => 'GPL',
              contact     => 'tg at x-net dot hu',
              url         => 'http://scripts.irssi.hu/',
             );

our (%psyc,$timeout,$width,$height,$debug,$witem_move);

Irssi::command_bind('save_psyc','save_session');
sub save_session {
  my ($upgrade) = @_;
  my $p = {pid => $$, upgrade => $upgrade, psyc => {}};
  Irssi::print 'Saving PSYC session';
  for my $t (keys %psyc) {
    $p->{psyc}->{$t} = $psyc{$t}->store;
  }

  #dumper($p);
  my $file = Irssi::get_irssi_dir."/psyc-session.$$";
  store $p, $file;
}

Irssi::command_bind('restore_psyc','restore_session');
sub restore_session {
  my ($upgrade) = @_;
  my $file = Irssi::get_irssi_dir."/psyc-session.$$";
  return unless -f $file;

  my $p = retrieve $file;
  return if $p->{pid} != $$ || $p->{upgrade} && !$upgrade;

  Irssi::print 'Restoring PSYC session';
  for my $t (keys %{$p->{psyc}}) {
    next unless ref $p->{psyc}->{$t} eq 'Net::PSYC::Irssi';
    $psyc{$t} = $p->{psyc}->{$t};
    if ($psyc{$t}->restore($upgrade)) {
      $psyc{$t}->set_terminal_size($width, $height);
    } else {
      delete $psyc{$t};
    }
  }

  $timeout = Irssi::timeout_add(250, \&sweep, '') unless $timeout;
  unlink $file;
}

Irssi::signal_add_first('session save', 'session_save');
sub session_save {
  debug("!! event session save");
  save_session(1);
  for my $t (keys %psyc) {
    #$psyc{$t}->server->disconnect;
  }
}

Irssi::signal_add_first('session restore', 'session_restore');
sub session_restore {
  restore_session(1);
}

sub sweep {
  for my $t (keys %psyc) {
    $psyc{$t}->sweep;
  }
}

sub server_connect {
  my ($server) = @_;
  my $t = $server->{tag};
  my $restore = exists $psyc{$t};
  $psyc{$t} ||= new Net::PSYC::Irssi();
  if ($psyc{$t}->lookup($server)) {
    $psyc{$t}->set_terminal_size($width, $height);
  } else {
    delete $psyc{$t};
  }
}

Irssi::signal_add_last('server looking', 'server_looking');
sub server_looking {
  my ($server) = @_;
  return if !$server || $server->{chat_type} ne 'PSYC';
  my $t = $server->{tag};
  debug("!! event server looking");
  server_connect($server);
}

Irssi::signal_add('server connecting', 'server_connecting');
sub server_connecting {
  my ($server, $ip, $restore) = @_;
  return if !$server || $server->{chat_type} ne 'PSYC';
  my $t = $server->{tag}; return unless $psyc{$t};
  debug("!! event server connecting");
  if ($psyc{$t}->connect($server, $restore)) {
    $timeout = Irssi::timeout_add(250, \&sweep, '') unless $timeout;
  } else {
    delete $psyc{$t};
  }
}

Irssi::signal_add('server connect failed', 'server_connect_failed');
sub server_connect_failed {
  my ($server) = @_;
  return if !$server || $server->{chat_type} ne 'PSYC';
  my $t = $server->{tag}; return unless $psyc{$t};
  debug("!! event server connect failed");
  delete $psyc{$t};
}

Irssi::signal_add('server disconnected', 'server_disconnected');
sub server_disconnected {
  my ($server) = @_;
  return if !$server || $server->{chat_type} ne 'PSYC';
  my $t = $server->{tag}; return unless $psyc{$t};
  debug("!! event server disconnected");
  delete $psyc{$t};
}

Irssi::signal_add_last('server quit', 'server_quit');
sub server_quit {
  my ($server, $msg) = @_;
  return if !$server || $server->{chat_type} ne 'PSYC';
  my $t = $server->{tag}; return unless $psyc{$t};
  debug("!! event server quit: $msg");
  $psyc{$t}->disconnect($msg, 1);
}

Irssi::signal_add('server sendmsg', 'server_sendmsg');
sub server_sendmsg {
  my ($server, $target, $msg, $target_type) = @_;
  return if !$server || $server->{chat_type} ne 'PSYC';
  my $t = $server->{tag}; return unless $psyc{$t};
  debug("!! event server sendmsg: $target, $msg, $target_type");
  $psyc{$t}->talk($target, $target_type, $msg);
}

Irssi::signal_add('message own_private', 'message_own_private');
sub message_own_private {
  my ($server, $msg, $target, $orig_target) = @_;
  return if !$server || $server->{chat_type} ne 'PSYC';
  my $t = $server->{tag}; return unless $psyc{$t};
  debug("!! event message own private: $server, $msg, $target, $orig_target");
  Irssi::signal_stop;
}

Irssi::signal_add('message own_public', 'message_own_public');
sub message_own_public {
  my ($server, $msg, $target) = @_;
  return if !$server || $server->{chat_type} ne 'PSYC';
  my $t = $server->{tag}; return unless $psyc{$t};
  debug("!! event message own public: $server, $msg, $target");
  Irssi::signal_stop;
}

Irssi::signal_add_first('query created', 'query_created');
sub query_created {
  my ($query, $auto) = @_;
  return if !$query || $query->{chat_type} ne 'PSYC';
  my $t = $query->{server}->{tag}; return unless $psyc{$t};
  debug("!! event query created: $query, $auto");
  $psyc{$t}->query_created($query, $auto);
}

Irssi::signal_add_first('channel created', 'channel_created');
sub channel_created {
  my ($channel, $auto) = @_;
  return if !$channel || $channel->{chat_type} ne 'PSYC';
  my $t = $channel->{server}->{tag}; return unless $psyc{$t};
  debug("!! event channel created: $channel (".$channel->{name}."), $auto");
  $psyc{$t}->channel_created($channel, $auto);
}

Irssi::command_bind('window item move','cmd_window_item_move');
sub cmd_window_item_move {
  my ($data, $server, $witem) = @_;
  debug("!! command window item move: $data, $server, $witem");
  $witem_move = 1;
}

Irssi::signal_add_first('window item new', 'window_item_new');
sub window_item_new {
  my ($window, $witem) = @_;
  if ($witem_move) { $witem_move = 0; return; }
  my $t = $witem->{server}->{tag}; return unless $psyc{$t};
  debug("!! event window item new: ".$witem->{name});
  $psyc{$t}->witem_created($witem);
}

Irssi::signal_add_first('window item remove', 'window_item_remove');
sub window_item_remove {
  my ($window, $witem) = @_;
  if ($witem_move) { $witem_move++; return; }
  my $t = $witem->{server}->{tag}; return unless $psyc{$t};
  debug("!! event window item remove: ".$witem->{name});
  $psyc{$t}->witem_removed($witem);
}

Irssi::signal_add('setup changed', 'setup_changed');
sub setup_changed {
  for my $t (keys %psyc) {
    $psyc{$t}->read_settings;
  }
  $debug = $Module::Reload::Debug = Irssi::settings_get_bool('psyc_debug');
}

Irssi::signal_add_first('default command', 'default_command');
sub default_command {
  my ($data, $server, $witem) = @_;
  return if !$server || $server->{chat_type} ne 'PSYC';
  my $t = $server->{tag}; return unless $psyc{$t};
  debug("!! event default command: $data, $server, $witem");
  if ($data =~ /^([^ ]+) *(.*) *$/) {
    my $target = $witem ? $witem->{name} : undef;
    $psyc{$t}->command($target, $1, $2);
    Irssi::signal_stop;
  }
}

Irssi::command_bind('psyc','default_command');
sub cmd_psyc {
  my ($data, $server, $witem) = @_;
  return if !$server || $server->{chat_type} ne 'PSYC';
  my $t = $server->{tag}; return unless $psyc{$t};
  debug("!! command psyc: $data, $server, $witem");
  my $name = $witem ? $witem->{name} : '';
  $psyc{$t}->execute($name, $data);
}

Irssi::command_bind('sendraw','cmd_sendraw');
sub cmd_sendraw {
  my ($data, $server, $witem) = @_;
  return if !$server || $server->{chat_type} ne 'PSYC';
  my $t = $server->{tag}; return unless $psyc{$t};
  debug("!! command sendraw: $data, $server, $witem");
  $data =~ /^(?:>([^ ]+) )?(\w+) +(.*) +({.*})/;
  my $vars = {};
  eval "local \$SIG{__DIE__}; \$vars = $4" if $4;
  if ($@) {
    $@ =~ /^(.*) at \(eval .*\) line .*$/;
    print $1;
  }

  my $name = $witem ? $witem->{name} : '';
  $psyc{$t}->sendmsg($1 || $name, $2, $3, $vars);
}

Irssi::command_bind('join','cmd_join');
Irssi::command_set_options('join', 'force');
#Irssi::command_bind('join force','cmd_join');
sub cmd_join {
  my ($data, $server, $witem) = @_;
  return if !$server || $server->{chat_type} ne 'PSYC' || (!$data && (!$witem || $witem->{type} ne 'CHANNEL'));
  my $t = $server->{tag}; return unless $psyc{$t};
  my $force = 0; $force = 1 if $data =~ /-force +/;
  $data =~ /(?:^| +)([^ ]+)$/;
  for (split ',', $1) {
    $psyc{$t}->enter($_ || $witem->{name}, $force);
  }
}

Irssi::command_bind('part','cmd_part');
sub cmd_part {
  my ($data, $server, $witem) = @_;
  return if !$server || $server->{chat_type} ne 'PSYC' || (!$data && (!$witem || $witem->{type} ne 'CHANNEL'));
  my $t = $server->{tag}; return unless $psyc{$t};
  debug("!! command part: $data, $server, $witem");
  $psyc{$t}->leave($data || $witem->{name});
}

Irssi::command_bind('topic','cmd_topic');
sub cmd_topic {
  my ($data, $server, $witem) = @_;
  return if !$server || $server->{chat_type} ne 'PSYC' || !$data || !$witem || $witem->{type} ne 'CHANNEL';
  my $t = $server->{tag}; return unless $psyc{$t};
  $data = '' if $data =~ /-delete */;
  debug("!! command topic: $data, $server, $witem");
  my ($target) = split ' ', $data;
  undef $target unless $server->window_item_find($target);
  $psyc{$t}->command($target || $witem->{name}, 'topic', $data);
}

Irssi::command_bind('me','cmd_me');
sub cmd_me {
  my ($data, $server, $witem) = @_;
  return if !$server || $server->{chat_type} ne 'PSYC';
  my $t = $server->{tag}; return unless $psyc{$t};
  debug("!! command me: $data, $server, $witem");
  my $type;
  $type = 0 if $witem->{type} eq 'CHANNEL';
  $type = 1 if $witem->{type} eq 'QUERY';
  $psyc{$t}->talk($witem->{name}, $type, '', $data) if defined $type;
}

Irssi::command_bind('away','cmd_away');
sub cmd_away {
  my ($data, $server, $witem) = @_;
  if ($data =~ /^ *-one *(.*)/) {
    $data = $1;
    return if !$server || $server->{chat_type} ne 'PSYC';
    my $t = $server->{tag}; return unless $psyc{$t};
    debug("!! command away: $data, $server, $witem");
    $psyc{$t}->away($data);
  } else {
    $data =~ s/^ *-all *//;
    for my $t (keys %psyc) {
      $psyc{$t}->away($data);
    }
  }
}

Irssi::command_bind('whois','cmd_whois');
sub cmd_whois {
  my ($data, $server, $witem) = @_;
  return if !$server || $server->{chat_type} ne 'PSYC';
  my $t = $server->{tag}; return unless $psyc{$t};
  debug("!! command whois: $data, $server, $witem");
  $data =~ /^ *([^ ]+)/;
  my $target = $1;
  $target = $witem->{name} if !$data && $witem && $witem->{type} eq 'QUERY';
  if ($target) {
    $psyc{$t}->command($target, 'whois', $target);
  }
}

Irssi::command_bind('examine','cmd_examine');
sub cmd_examine {
  my ($data, $server, $witem) = @_;
  return if !$server || $server->{chat_type} ne 'PSYC';
  my $t = $server->{tag}; return unless $psyc{$t};
  debug("!! command examine: $data, $server, $witem");
  $data =~ /^ *([^ ]+)/;
  my $target = $1;
  $target = $witem->{name} if !$data && $witem && $witem->{type} eq 'QUERY';
  if ($target) {
    $psyc{$t}->command(undef, 'examine', $data);
  }
}

Irssi::command_bind('surf','cmd_surf');
sub cmd_surf {
  my ($data, $server, $witem) = @_;
  return if !$server || $server->{chat_type} ne 'PSYC';
  my $t = $server->{tag}; return unless $psyc{$t};
  debug("!! command surf: $data, $server, $witem");
  $data =~ /^ *([^ ]+)/;
  my $target = $1;
  $target = $witem->{name} if !$data && $witem && $witem->{type} eq 'QUERY';
  if ($target) {
    $psyc{$t}->command(undef, 'surf', $data);
  }
}

Irssi::command_bind('nick','cmd_nick');
sub cmd_nick {
  my ($data, $server, $witem) = @_;
  return if !$server || $server->{chat_type} ne 'PSYC' || !$witem;
  my $t = $server->{tag}; return unless $psyc{$t};
  debug("!! command nick: $data, $server, $witem");
  $data = '' if $data =~ /^[-0]$/;
  $psyc{$t}->nick($witem->{name}, $data);
}

Irssi::command_bind('perl','cmd_perl');
sub cmd_perl {
  my ($data, $server, $witem) = @_;
  debug("!! command perl: $data, $server, $witem");
  my ($k) = keys %psyc;
  my $p; $p = $psyc{$k} if $k;

  eval "local \$SIG{__DIE__}; $data";
  if ($@) {
    $@ =~ /^(.*) at \(eval .*\) line .*$/;
    print $1;
  }
}

Irssi::command_bind('quit','cmd_quit');
sub cmd_quit {
  my ($data, $server, $witem) = @_;
  debug("!! command quit: $data, $server, $witem");
  my $file = Irssi::get_irssi_dir."/psyc-session.$$";
  unlink $file if -f $file;
}

sub getopts {
  my ($data) = @_;
  my $o = {};
  my $a = [];
  while ($data =~ /(?:-([a-z0-9_!]+) *([^ ]+)|([^ ]+))/g) {
    $o->{$1} = $2 if $1;
    push @$a, $3 if $3;
  }
  return ($o,$a);
}

Irssi::command_bind('network psyc', 'cmd_network_psyc');
Irssi::command_bind('network psyc add', 'cmd_network_psyc');
Irssi::command_bind('network psyc remove', 'cmd_network_psyc');
Irssi::command_bind('network psyc list', 'cmd_network_psyc');
sub cmd_network_psyc {
  my ($data, $server, $witem) = @_;
  debug("!! command network psyc: $data, $server, $witem");
  my $chatnet;
  if ($data =~ /^add ([^ ]+)$/) {
    my $name = $1;
    if ($chatnet = Irssi::chatnet_find($name)) {
    } else {
      $chatnet = new Irssi::Chatnet({name => $name, chat_type => 'PSYC'});
    }
    Irssi::printformat(MSGLEVEL_CLIENTNOTICE, 'network_added', $chatnet->{name}) if $chatnet;
  } elsif ($data =~ /^remove ([^ ]+)/) {
    my $name = $1;
    if ($chatnet = Irssi::chatnet_find($name)) {
      $name = $chatnet->{name};
      $chatnet->remove;
      Irssi::printformat(MSGLEVEL_CLIENTNOTICE, 'network_removed', $name);
    } else {
      Irssi::printformat(MSGLEVEL_CLIENTNOTICE, 'network_not_found', $name);
    }
  } elsif ($data =~ /^list/) {
    cmd_network_list($data, $server, $witem, 1);
  }
}

Irssi::command_bind_last('network list', 'cmd_network_list');
sub cmd_network_list {
  my ($data, $server, $witem, $nosep) = @_;
  debug("!! command network list: $data, $server, $witem");
  Irssi::printformat(MSGLEVEL_CLIENTCRAP, 'network_separator') unless $nosep;
  Irssi::printformat(MSGLEVEL_CLIENTCRAP, 'network_header');
  for my $s (Irssi::chatnets) {
    next if $s->{chat_type} ne 'PSYC';
    my $o = '';
    $o .= "nick: $s->{nick}, " if $s->{nick};
    $o .= "username: $s->{username}, " if $s->{username};
    $o .= "realname: $s->{realname}, " if $s->{realname};
    $o .= "own_host: $s->{own_host}, " if $s->{own_host};
    $o .= "autosendcmd: $s->{autosendcmd}, " if $s->{autosendcmd};
    Irssi::printformat(MSGLEVEL_CLIENTCRAP, 'network_line', $s->{name}, $o);
  }
  Irssi::printformat(MSGLEVEL_CLIENTCRAP, 'network_footer');
}

Irssi::command_bind('server psyc','cmd_server_psyc');
Irssi::command_bind('server psyc add','cmd_server_psyc');
Irssi::command_bind('server psyc remove','cmd_server_psyc');
Irssi::command_bind('server psyc list','cmd_server_psyc');
sub cmd_server_psyc {
  my ($data, $server, $witem) = @_;
  debug("!! command server psyc: $data, $server, $witem");
  my $setup;
  if ($data =~ /^add +(.+)$/) {
    my $o = {};
    my ($ret, $args) = Getopt::Long::GetOptionsFromString($1, $o, 'network=s','host=s','4','6','auto','noauto',
                                                          'ssl','ssl_cert=s','ssl_pkey=s','ssl_verify=s','ssl_cafile=s','ssl_capath=s');
    dumper($ret, $args, $o);
    my ($addr,$pass) = @$args;
    my $s = {address => $addr, chat_type => 'PSYC'};
    $s->{password} = $pass if $pass;
    $s->{family} = AF_INET if exists $o->{4};
    $s->{family} = AF_INET6 if exists $o->{6};
    $s->{autoconnect} = 1 if exists $o->{auto};
    $s->{autoconnect} = 0 if exists $o->{noauto};
    $s->{use_ssl} = 1 if exists $o->{ssl};

    my %map = (host => 'own_host', network => 'chatnet');
    for (keys %map) {
      $s->{$map{$_}} = $o->{$_} if exists $o->{$_};
    }
    for (qw/ssl_cert ssl_pkey ssl_verify ssl_cafile ssl_capath/) {
      $s->{$_} = $o->{$_} if exists $o->{$_}
    }

    my $u = parse_uniform($s->{address});
    unless (ref $u && $u->{host} && $u->{object}) {
      Irssi::print("$s->{address}: invalid uniform");
      return;
    }

    if ($s->{chatnet}) {
      my $chatnet = Irssi::chatnet_find($s->{chatnet});
      unless ($chatnet && $chatnet->{chat_type} eq 'PSYC') {
        Irssi::print("$s->{chatnet} is not a PSYC network");
        return;
      }
    }

    if ($setup = Irssi::server_setup_find($addr)) {
      $setup->update($s);
    } else {
      if (!$s->{chatnet}) {
        Irssi::print("You have to specify a network");
        return;
      }
      $setup = new Irssi::ServerSetup($s);
    }

    dumper($s);
    dumper($setup);
    Irssi::printformat(MSGLEVEL_CLIENTNOTICE, 'setupserver_added', $setup->{address}) if $setup;
  } elsif ($data =~ /^remove ([^ ]+)/) {
    my $addr = $1;
    if ($setup = Irssi::server_setup_find($addr)) {
      $addr = $setup->{address};
      $setup->remove;
      Irssi::printformat(MSGLEVEL_CLIENTNOTICE, 'setupserver_removed', $addr);
    } else {
      Irssi::printformat(MSGLEVEL_CLIENTNOTICE, 'setupserver_not_found', $addr);
    }
  } elsif ($data =~ /^list/) {
    cmd_server_list($data, $server, $witem, 1);
  }
}

Irssi::command_bind_last('server list', 'cmd_server_list');
sub cmd_server_list {
  my ($data, $server, $witem, $nosep) = @_;
  Irssi::printformat(MSGLEVEL_CLIENTCRAP, 'setupserver_separator') unless $nosep;
  debug("!! command server list: $data, $server, $witem");
  Irssi::printformat(MSGLEVEL_CLIENTCRAP, 'setupserver_header');
  for my $s (Irssi::setupservers) {
    next if $s->{chat_type} ne 'PSYC';
    my $o = '';
    $o .= "(pass), " if $s->{password};
    $o .= "autoconnect, " if $s->{autoconnect};
    $o .= "noproxy, " if $s->{no_proxy};
    $o .= "ssl, " if $s->{use_ssl};
    $o .= "ssl_cert: $s->{ssl_cert}, " if $s->{ssl_cert};
    $o .= "ssl_pkey: $s->{ssl_pkey}, " if $s->{ssl_pkey};
    $o .= "ssl_verify, " if $s->{ssl_verify};
    $o .= "ssl_cafile: $s->{ssl_cafile}, " if $s->{ssl_cafile};
    $o .= "ssl_capath: $s->{ssl_capath}, " if $s->{ssl_capath};
    $o .= "host: $s->{own_host}, " if $s->{own_host};
    Irssi::printformat(MSGLEVEL_CLIENTCRAP, 'setupserver_line', $s->{address}, $s->{chatnet}, $o);
  }
  Irssi::printformat(MSGLEVEL_CLIENTCRAP, 'setupserver_footer');
}

Irssi::signal_add('window changed', 'window_changed');
sub window_changed {
  my ($new, $old) = @_;
  debug("!! signal window changed: $new, $old");
  if ($new && $new->{active} && $new->{active}->{server}) {
    my $witem = $new->{active};
    my $server = $witem->{server};
    my $t = $server->{tag};
    $psyc{$t}->window_changed_to($witem->{name}) if $psyc{$t};
  }
  if ($old && $old->{active} && $old->{active}->{server}) {
    my $witem = $old->{active};
    my $server = $witem->{server};
    my $t = $server->{tag};
    $psyc{$t}->window_changed_away($witem->{name}) if $psyc{$t};
  }
}

sub get_terminal_size {
  ($width, $height) = GetTerminalSize();
}

Irssi::signal_add('mainwindow resized', 'mainwindow_resized');
sub mainwindow_resized {
  get_terminal_size;
  for my $t (keys %psyc) {
    $psyc{$t}->set_terminal_size($width, $height);
  }
}

sub witem_printformat {
  my ($witem) = shift;
  $witem->printformat(@_);
}

sub server_printformat {
  my ($server) = shift;
  $server->printformat(@_);
}

sub debug {
  return unless $debug;
  my @p; for (@_) {push @p, $_ if defined $_};
  print STDERR "@p\n";
}

sub dumper {
  return unless $debug;
  debug(Dumper(@_));
}

Irssi::signal_add('signal', 'signal');
sub signal {
  my $sig = shift;
  debug("signal: @_");
  #debug("signal: $sig(".join(', ',@_).")");
}

sub LOAD {
  Irssi::theme_register([
                         'setupserver_separator' => ' ',
                         'setupserver_header' => '%#PSYC UNI                   Network    Settings',
                         'setupserver_line' => '%#%|$[!26]0 $[10]1 $2',
                         'setupserver_footer' => '',
                         'setupserver_added' => 'Server {server $0} saved',
                         'setupserver_removed' => 'Server {server $0} removed',
                         'setupserver_not_found' => 'Server {server $0} not found',
                         'network_separator' => ' ',
                         'network_header' => '%#PSYC Networks:',
                         'network_line' => '%#$0: $1',
                         'network_footer' => '',
                         'network_added' => 'Network $0 saved',
                         'network_removed' => 'Network $0 removed',
                         'network_not_found' => 'Network $0 not found',
                        ]);

  Irssi::settings_add_str($IRSSI{'name'}, 'psyc_friends_channel', '&friends');
  Irssi::settings_add_bool($IRSSI{'name'}, 'psyc_friends_auto_update', 1);
  Irssi::settings_add_bool($IRSSI{'name'}, 'psyc_debug', 0);
  Irssi::settings_add_bool($IRSSI{'name'}, 'psyc_display_speakaction', 1);

  Irssi::command('statusbar topic enable');

  $debug = $Module::Reload::Debug = Irssi::settings_get_bool('psyc_debug');

  Module::Reload->check;
  get_terminal_size;
  restore_session;

  Net::PSYC::Irssi->LOAD;
  Net::PSYC::Irssi::Window->LOAD;
}

sub UNLOAD {
  Net::PSYC::Irssi::Window->UNLOAD;
  Net::PSYC::Irssi->UNLOAD;

  Irssi::command('statusbar topic enable');

  save_session;
}

$SIG{__WARN__} = sub {
  my $msg = "@_";
  $msg =~ s/\n$//s;
  print $msg;
  return 1;
};

LOAD;
