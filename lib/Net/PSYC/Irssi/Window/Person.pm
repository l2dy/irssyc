package Net::PSYC::Irssi::Window::Person;
use strict;
use warnings;
use base 'Net::PSYC::Irssi::Window';

use Net::PSYC qw(psyctext);
use Irssi;

sub new {
  my ($class, $irssi, $uni, $name, $auto, $witem) = @_;
  my $s = Net::PSYC::Irssi::Window->new($irssi, $uni, $name, 'person', $auto, $witem);
  bless $s, $class;
  $s->debug('#### NEW PERSON', $name, $uni);
  return $s;
}

sub restore {
  my ($s, $irssi) = @_;
  $s = $s->SUPER::restore($irssi);

  if (ref $s->{witem} eq 'HASH') {
    undef $s->{witem};
    $s->query_create($s->{uni}, $s->{name}, 1);
  }
  return $s;
}

sub msg {
  my $s = shift;
  my ($uni, $name, $mc, $data, $vars, $tag) = @_;
  my $print = 0;
  $_ = $mc;
 SWITCH: {
    /^(_message_private|_notice_action)/ && do {
      unless ($s->print_msg(@_)) {
        $s->query_create($s->{uni}, $s->{name}, 1);
      }
      last SWITCH;
    };
    /^_message_echo_private/ && do {
      last SWITCH;
    };
    /^_request_attention_wake/ && do {
      $vars->{_beep} = ' ';
      my $msg = psyctext($data, $vars);
      $s->print_notice($msg, undef, MSGLEVEL_HILIGHT);
      last SWITCH;
    };
    $print = 1;
  }

  $s->SUPER::msg(@_, $print);
}

sub query_create {
  my ($s, $uni, $nick, $auto) = @_;
  $s->debug('>> query_create:', $uni, $nick);
  return 0 if $s->witem;
  my $query = {name => $uni, visible_name => $nick, address => $uni};
  $query = new Irssi::Psyc::Query($s->server, $query, $auto);
  $s->{witem} = $query;
  return 1;
}

sub nick_change {
  my ($s, $old, $new) = @_;
  $s->debug('>> person nick change:', $old, $new);
  return 0 if $old ne $s->{name} || !$s->witem;

  $s->{name} = $new;
  $s->{witem} = $s->witem->update({visible_name => $new});
  Irssi::signal_emit('query nick changed', $s->witem, $old);

  return 1;
}

sub print_msg_plain {
  my ($s, $msg, $nick, $local, $uni, $action) = @_;
  my $level = MSGLEVEL_MSGS;
  my $act = $action ? '_action' : '';
  $s->print_format($level, "msg_private_query$act", $nick, $msg, $action);
  Irssi::signal_emit('psyc message private', $s->server, $msg, $nick, $uni);
}

sub print_msg_own {
  my ($s, $msg) = @_;
  my $level = MSGLEVEL_MSGS | MSGLEVEL_NOHILIGHT | MSGLEVEL_NO_ACT;
  my $action = $s->irssi->{action} ? ' '.$s->irssi->{action} : '';
  my $act = $action ? '_action' : '';
  $s->print_format($level, "own_msg_private_query$act", $s->irssi->{nick}, $msg, $action);
  #Irssi::signal_emit('message own_private', $s->server, $msg, $s->{name}, $s->{uni});
}

sub print_msg_action {
  my ($s, $msg, $nick) = @_;
  my $level = MSGLEVEL_MSGS | MSGLEVEL_ACTIONS;
  $s->print_format($level, 'action_private_query', $nick, $msg);
}

sub print_msg_action_own {
  my ($s, $msg) = @_;
  my $level = MSGLEVEL_MSGS | MSGLEVEL_ACTIONS | MSGLEVEL_NOHILIGHT | MSGLEVEL_NO_ACT;
  $s->print_format($level, 'own_action', $s->{irssi}->{nick}, $msg);
  #Irssi::signal_emit('message psyc action', $witem, $action);
}

sub LOAD {
  Irssi::signal_register({'psyc message private' => [qw(Irssi::Server string string string)]});
}

1;
