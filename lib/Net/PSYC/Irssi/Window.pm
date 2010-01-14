package Net::PSYC::Irssi::Window;
use strict;
use warnings;

use base 'Net::PSYC::Irssi::Common';
use Net::PSYC qw(psyctext make_uniform same_host);
use Irssi;
use POSIX qw(strftime);

sub new {
  my ($class, $irssi, $uni, $name, $type, $auto, $witem) = @_;
  $class = ref $class || $class;
  my $s = {
           irssi => $irssi,
           server => $irssi->{server},
           settings => $irssi->{settings},
           aliases => $irssi->{aliases},
           raliases => $irssi->{raliases},
           server_uni => $irssi->{server_uni},
           server_host => $irssi->{server_host},
           uni => $uni,
           name => $name,
           type => $type,
           auto => $auto,
           witem => $witem,
           msgbuf => [],
          };
  bless $s, $class;
  return $s;
}

sub store {
  my ($s) = @_;
  my $h = {};
  for (keys %$s) {
    $s->store_one($s, $h, $_) unless /^(irssi|server|settings|aliases|raliases)$/;
  }
  bless $h, ref $s;
  return $h;
}

sub restore {
  my ($s, $irssi) = @_;
  $s->{irssi} = $irssi;
  $s->{$_} = $irssi->{$_} for (qw/server settings aliases raliases/);
  if (ref $s->{witem} eq 'HASH' && $s->server) {
    my $w = $s->server->window_item_find($s->{witem}->{name});
    $s->{witem} = $w if $w;
  }
  return $s;
}

sub msg {
  my ($s, $uni, $name, $mc, $data, $vars, $tag, $print) = @_;
  my $msg = psyctext($data, $vars);
  #$s->debug("** msg: $mc, $uni, $name, $data, [".$s->{uni}.' : '.$s->{type}.']');
  $s->debug('** msg:', $mc, $uni, '->', $s->{uni}, "[$s->{type}]", $msg);
  my $print2 = 0;
  $_ = $mc;
 SWITCH: {
    $print2 = 1;
  }

  #$s->dumper($vars);
  #$s->dumper($tag);
  $s->print_notice($msg) if $print && $print2 && (!$tag || !$tag->{params}->{noprint});
  delete $s->irssi->{tags}->{$tag} if $tag && exists $s->irssi->{tags}->{$tag};

  Irssi::signal_emit('psyc message', $mc, $uni, $name, $data) if $mc;
}

sub print_format {
  my ($s, $level, $format, @msg) = @_;
  $s->debug('>> print_format:',$format);
  for (0..@msg) { $msg[$_] = '' unless defined $msg[$_] }
  my $target = $s->witem ? $s->witem->{name} : '';
  $msg[$_] = Irssi::recode_in($s->server, $msg[$_], $target) for 0..@msg-1;
  #if ($s->recode) {
  #  $msg[$_] = $s->recode->convert($msg[$_]) || $msg[$_] for 0..@msg-1;
  #}

  if ($s->witem) {
    $s->witem->printformat($level, $format, @msg);
  } elsif ($s->server) {
    $s->server->printformat('', $level, $format, @msg);
  }
}

sub print_notice {
  my ($s, $msg, $format, $level) = @_;
  return unless $msg;
  $level ||= MSGLEVEL_SNOTES;
  if (!$format) {
    if ($s->witem) {
      $format = 'psyc_msg';
      #$format = 'notice_public' if $s->witem->{type} eq 'CHANNEL';
      #$format = 'notice_private' if $s->witem->{type} eq 'QUERY';
    } else {
      $format = 'psyc_msg_status';
    }
  }
  my @msgs = split /\r\n|\r|\n/, $msg;
  for $msg (@msgs) {
    $s->print_format($level, $format, $msg);
  }
}

sub print_log {
  my ($s, $msg, $level) = @_;
  return unless $msg && $s->witem;
  $level ||= MSGLEVEL_CRAP;
  my $target = $s->witem ? $s->witem->{name} : '';
  $msg = Irssi::recode_in($s->server, $msg, $target);
  #$msg = $s->recode->convert($msg) || $msg if $s->recode;
  my $w = $s->witem->window;
  unless ($w) {
    $s->debug('!!!!!!!!!!! z0mg no window @ print_log !!!!!!!!!!!');
    $s->dumper(Irssi::windows);
    return;
  }
  my $line = $w->view->get_lines;
  while ($line && $line->next) {
    $line = $line->next;
  }
  $w->gui_printtext_after($line, MSGLEVEL_CRAP, "$msg\n");
  $w->view->redraw;
}

sub print_msg {
  my $s = shift;
  my ($uni, $name, $mc, $data, $vars) = @_;
  unless ($s->witem) {
    #delay msg display until window is created
    push @{$s->{msgbuf}}, \@_;
    return 0;
  }

  $s->debug('>> print_msg:', $mc, $name, $data, $mc, $uni);
  if ($vars->{_time_log} || $vars->{_time_place}) {
    $s->print_msg_log(@_);
  } else {
    my $local = $vars->{_nick_local};
    if ($data eq '' && $vars->{_action} || $mc =~ /^_notice_action/) {
      my $msg = $vars->{_action};
      unless ($msg) {
        $data =~ s/[\r\n].*//;
        if ($data =~ /^\[_nick\] (.*)/) {
          $msg = psyctext($1, $vars);
        } else {
          $msg = psyctext($data, $vars);
        }
      }
      if ($uni eq $s->irssi->{uni}) {
        $s->print_msg_action_own($msg, $local) if $mc =~ /^_notice_action/;
      } else {
        $s->print_msg_action($msg, $name, $local);
      }
    } else {
      if ($uni eq $s->irssi->{uni}) {
        $s->print_msg_own($data, $local) if $mc =~ /^_notice_action/;
      } else {
        $s->print_msg_plain($data, $name, $local, $uni, $vars->{_action});
      }
    }
  }
  return 1;
}

sub print_msg_log {
  my ($s, $uni, $name, $mc, $data, $vars) = @_;
  my $log = $vars->{_time_log} || $vars->{_time_place};
  my $time = strftime "%b %d %H:%M", localtime $log;
  #my $nick = $s->get_name($vars);
  my $msg = "<$name> $data";
  if ($mc =~ /^_notice_action/) {
    $msg = '* '.psyctext($data, $vars);
  } elsif (!$data && $vars->{_action}) {
    $msg = "* $name ".$vars->{_action};
  } elsif ($vars->{_action}) {
    $msg = "<$name ".$vars->{_action}."> $data";
  }
  $s->print_log("$time $msg");
}

sub print_msg_plain {
  my ($s, $msg, $nick, $addr) = @_;
}

sub print_msg_own {
  my ($s, $msg) = @_;
}

sub print_msg_action {
  my ($s, $msg, $nick) = @_;
}

sub print_msg_action_own {
  my ($s, $msg, $nick) = @_;
}

sub talk {
  my ($s, $msg, $action) = @_;
  $s->debug('>> window talk:', $msg, $action);
  if ($action) {
    $s->print_msg_action_own($action);
  } else {
    $s->print_msg_own($msg);
  }
}

sub witem_created {
  my ($s, $witem) = @_;
  $s->debug('>>> witem_created:', $witem, $s->{uni}, @{$s->{msgbuf}});
  $s->{witem} = $witem;

  if (@{$s->{msgbuf}}) {
    for my $m (@{$s->{msgbuf}}) {
      $s->print_msg(@$m); #$m->{mc}, $m->{data}, $m->{vars}
    }
    $s->{msgbuf} = [];
  }
}

sub witem_removed {
  my ($s) = @_;
  undef $s->{witem};
}

sub init {}
sub destroy {}
sub nick_change {}

sub irssi  { shift->{irssi} }
sub server { shift->{irssi}->{server} }
#sub recode { shift->{irssi}->{recode} }
sub witem  { shift->{witem} }

sub LOAD {
  Irssi::signal_register({'psyc message' => [qw(string string string string)]});

  my $hi = Irssi::settings_get_str('hilight_color');
  Irssi::theme_register([
                         'own_msg' => '{ownmsgnick $2 {ownnick $0}}$1',
                         'own_msg_action' => '{ownmsgnick $2 {ownnick $0} {ownnick_action $4}}$1',
                         'own_msg_masq' => '{ownmsgnick_masq $2 {ownnick $0} {ownnick_masq $3}}$1',
                         'own_msg_masq_action' => '{ownmsgnick_masq $2 {ownnick $0} {ownnick_masq $3} {ownnick_action $4}}$1',
                         #'own_msg_channel' => '{ownmsgnick $3 {ownnick $0}{msgchannel $1}}$2',
                         #'own_msg_private' => '{ownprivmsg msg $0}$1',
                         'own_msg_private_query' => '{ownprivmsgnick {ownprivnick $0}}$1',
                         'own_msg_private_query_action' => '{ownprivmsgnick {ownprivnick $0} {ownprivnick_action $2}}$1',
                         'pubmsg' => '{pubmsgnick $2 {pubnick $0}}$1',
                         'pubmsg_hilight' => '{pubmsghinick '.$hi.' $2 $0}$1',
                         'pubmsg_action' => '{pubmsgnick $2 {pubnick $0} {pubnick_action $4}}$1',
                         'pubmsg_action_hilight' => '{pubmsghinick '.$hi.' $2 $0 {pubnick_action $4}}$1',
                         'pubmsg_masq' => '{pubmsgnick_masq $2 {pubnick $0} {pubnick_masq $3}}$1',
                         'pubmsg_masq_hilight' => '{pubmsghinick_masq '.$hi.' $2 $0 {pubnick_masq '.$hi.'$3%n}}$1',
                         'pubmsg_masq_action' => '{pubmsgnick_masq $2 {pubnick $0} {pubnick_masq $3} {pubnick_action $4}}$1',
                         'pubmsg_masq_action_hilight' => '{pubmsghinick_masq '.$hi.' $2 $0 {pubnick_masq '.$hi.'$3%n} {pubnick_action $4}}$1',
                         #'pubmsg_channel' => '{pubmsgnick $3 {pubnick $0}{msgchannel $1}}$2',
                         #'pubmsg_me' => '{pubmsgmenick $2 {menick $0}}$1',
                         #'pubmsg_me_channel' => '{pubmsgmenick $3 {menick $0}{msgchannel $1}}$2',
                         #'pubmsg_hilight' => '{pubmsghinick $0 $3 $1}$2',
                         #'pubmsg_hilight_channel' => '{pubmsghinick $0 $4 $1{msgchannel $2}}$3',
                         #'msg_private' => '{privmsg $0 $1}$2',
                         'msg_private_query' => '{privmsgnick {privnick $0}}$1',
                         'msg_private_query_action' => '{privmsgnick {privnick $0} {privnick_action $2}}$1',

                         #'own_notice' => '{ownnotice notice $0}$1',
                         'own_action' => '{ownaction $0}$1',
                         'own_action_masq' => '{ownaction_masq $0 $2}$1',
                         #'own_action_target' => '{ownaction_target $0 $2}$1',
                         #'action_private' => '{pvtaction $0}$1',
                         'action_private_query' => '{pvtaction_query $0}$1',
                         'action_public' => '{pubaction $0}$1',
                         'action_public_hilight' => '{pubaction '.$hi.'$0%n}$1',
                         'action_public_masq' => '{pubaction_masq $0 $2}$1',
                         'action_public_masq_hilight' => '{pubaction_masq '.$hi.'$0%n '.$hi.'$2%n}$1',
                         #'action_public_channel' => '{pubaction $0{msgchannel $1}}$2',

                         'notice_server' => '{servernotice $0}$1',
                         'notice_public' => '{notice $0{pubnotice_channel $1}}$2',
                         'notice_private' => '{notice $0{pvtnotice_host $1}}$2',

                         'psyc_msg' => '{line_start} $0',
                         'psyc_msg_status' => '%[T]{line_start} $0',

                         'join_msg' => '{channick_hilight $0} {chanhost_hilight $1} $2',
                         'part_msg' => '{channick $0} {chanhost $1} $2',

                         'list_members' => '$0%K$1%n$2',
                        ]);
}

sub UNLOAD {
  Irssi::theme_unregister;
}

1;
