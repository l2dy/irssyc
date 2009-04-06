package Net::PSYC::Irssi::Window::Place;
use strict;
use warnings;
use base 'Net::PSYC::Irssi::Window::Channel';

use Net::PSYC qw(psyctext);
use Irssi;

sub new {
  my ($class, $irssi, $uni, $name, $auto, $witem) = @_;
  my $s = Net::PSYC::Irssi::Window->new($irssi, $uni, $name, 'place', $auto, $witem);
  $s->{members} ||= {};
  $s->{nick_local} ||= undef;
  bless $s, $class;
  $s->debug('#### NEW PLACE', $name, $uni);
  return $s;
}

sub restore {
  my ($s, $irssi) = @_;
  $s->SUPER::restore($irssi);

  if (ref $s->{witem} eq 'HASH') {
    undef $s->{witem};
    $s->channel_create($s->{uni}, $s->{name}, 1);
  }

   $s->insert_nicks;
  return $s;
}

sub init {
  my ($s) = @_;
  $s->debug('>> place init');
  $s->channel_create($s->{uni}, $s->{name}, $s->{auto});
}

sub msg {
  my $s = shift;
  my ($uni, $name, $mc, $data, $vars, $tag) = @_;
  my $print = 0;
  $_ = $mc;
 SWITCH: {
    /^_notice_place_topic$/ && do {
      my $topic = $vars->{_topic};
      $topic = '' if $mc =~ /^_notice_place_topic_removed$/;
      if ($s->witem) {
        #$topic = $s->recode->convert($topic) || $topic if $s->recode;
        $topic = Irssi::recode_in($s->server, $topic, $s->{uni});
        $s->{witem} = $s->witem->update({topic => $topic, topic_by => $vars->{_nick}, topic_time => time});
        Irssi::signal_emit("channel topic changed", $s->witem);
        Irssi::signal_emit('message topic', $s->server, $s->witem->{name}, $topic, $vars->{_nick}, $uni);
      }
      last SWITCH;
    };
    /^_status_place_topic$/ && do {
      last SWITCH unless exists $s->{members};
      if ($s->witem) {
        last SWITCH if $s->witem->{topic} eq $vars->{_topic};
        my $topic = $vars->{_topic};
        #$topic = $s->recode->convert($topic) || $topic if $s->recode;
        $topic = Irssi::recode_in($s->server, $topic, $s->{uni});
        $s->{witem} = $s->witem->update({topic => $topic, topic_by => $vars->{_nick}, topic_time => time});
        Irssi::signal_emit("channel topic changed", $s->witem);
      }
      $print = 1;
      last SWITCH;
    };
    /^(_message(_echo)?_public|_notice_action)/ && do {
      $s->print_msg(@_);
      last SWITCH;
    };
    /^_message_echo_public/ && do {
      last SWITCH;
    };
    /^_echo_place_leave$/ && do {
      $s->destroy;
      last SWITCH;
    };
    /^_echo_place_enter$/ && do {
      last SWITCH;
    };
    /^_notice_place_leave/ && do {
      delete $s->{members}->{$uni};
      my $nick = $s->witem->nick_find($name);
      $s->witem->nick_remove($nick) if $nick;
      Irssi::signal_emit('message part', $s->server, $s->{name}, $name, $uni);
      last SWITCH;
    };
    /^_notice_place_enter/ && do {
      my $nick = {nick => $name, host => $uni};
      $s->{members}->{$uni} = $s->nick_insert($nick);
      Irssi::signal_emit('message join', $s->server, $s->{name}, $name, $uni);
      last SWITCH;
    };
    /^_status_place_members$/ && do {
      $s->{members} = {};
      my $max = 0;
      for (0 .. @{$vars->{_list_members}} - 1) {
        my $m = $vars->{_list_members}->[$_];
        my $n = $vars->{_list_members_nicks}->[$_];
        my $nick = {nick => $n, host => $m};
        $s->{members}->{$m} = $s->nick_insert($nick);
        $max = length $n if $max < length $n;
      }
      $s->sync;
      for my $m (sort keys %{$s->{members}}) {
        my $nick = $s->{members}->{$m};
        my $dots = '.' x ($max + 5 - length $nick->{nick});
        $s->print_format(MSGLEVEL_CLIENTCRAP, 'list_members', $nick->{nick}, $dots, $nick->{host});
      }
      last SWITCH;
    };
    /^_notice_place_masquerade/ && do {
      $s->{nick_local} = $vars->{_nick_local} if $uni eq $s->irssi->{uni};
      $print = 1;
      last SWITCH;
    };
    /^_echo_place_nick_removed/ && do {
      undef $s->{nick_local};
      $print = 1;
      last SWITCH;
    };
    $print = 1;
  }

  $s->SUPER::msg(@_, $print);
}

sub print_msg_plain {
  my ($s, $msg, $nick, $local, $uni, $action) = @_;
  $s->debug('>> print msg plain:', $msg, $nick, $local);
  my $mode = '';
  my $act = $action && $s->{settings}->{display_speakaction} ? '_action' : '';
  my $masq = $local ? '_masq' : '';
  my $hi = '';
  my $level = MSGLEVEL_PUBLIC;
  my $n = $s->server->{nick};
  if ($msg =~ /\b$n\b/i) {
    $level |= MSGLEVEL_HILIGHT;
    $hi = '_hilight';
  }
  $s->print_format($level, "pubmsg$masq$act$hi", $nick, $msg, $mode, $local, $action);
  #print "msg: $msg, $nick, $uni, $s->{uni}";
  Irssi::signal_emit('psyc message public', $s->server, $msg, $nick, $uni, $s->{uni});
}

sub print_msg_own {
  my ($s, $msg, $local) = @_;
  $local ||= $s->{nick_local} || '';
  $s->debug('>> print msg own:', $msg, $local);
  my $mode = '';
  my $action = $s->irssi->{action};
  my $act = $action && $s->{settings}->{display_speakaction} ? '_action' : '';
  my $masq = $local ? '_masq' : '';
  my $level = MSGLEVEL_PUBLIC | MSGLEVEL_NOHILIGHT | MSGLEVEL_NO_ACT;
  $s->print_format($level, "own_msg$masq$act", $s->irssi->{nick}, $msg, $mode, $local, $action);
  #Irssi::signal_emit('message own_public', $s->server, $msg, $s->{name}, $s->{uni});
}

sub print_msg_action {
  my ($s, $msg, $nick, $local) = @_;
  $s->debug('>> print msg action:', $msg, $nick, $local);
  my $masq = $local ? '_masq' : '';
  my $hi = '';
  my $level = MSGLEVEL_PUBLIC | MSGLEVEL_ACTIONS;
  my $n = $s->server->{nick};
  if ($msg =~ /\b$n\b/i) {
    $level |= MSGLEVEL_HILIGHT;
    $hi = '_hilight';
  }
  $s->print_format($level, "action_public$masq$hi", $nick, $msg, $local);
}

sub print_msg_action_own {
  my ($s, $msg, $local) = @_;
  $local ||= $s->{nick_local} || '';
  $s->debug('>> print msg action own:', $msg, $local);
  my $masq = $local ? '_masq' : '';
  my $level = MSGLEVEL_PUBLIC | MSGLEVEL_ACTIONS | MSGLEVEL_NOHILIGHT | MSGLEVEL_NO_ACT;
  $s->print_format($level, "own_action$masq", $s->irssi->{nick}, $msg, $local);
}

sub destroy {
  my ($s) = @_;
  return unless $s->witem;
  my $win = $s->witem->window;
  $s->witem->destroy;
  $win->destroy;
}

sub witem_removed {
  my ($s) = @_;
  $s->SUPER::witem_removed;
  $s->irssi->command(undef, 'leave', $s->{uni}) unless $s->{noleave};
}

1;
