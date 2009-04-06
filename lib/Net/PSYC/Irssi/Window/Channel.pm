package Net::PSYC::Irssi::Window::Channel;
use strict;
use warnings;
use base 'Net::PSYC::Irssi::Window';

use Irssi;

sub channel_create {
  my ($s, $uni, $name, $auto) = @_;
  $s->debug('>> channel_create:', $uni, $name, $auto);
  return 0 if !$uni || $s->witem;
  $name ||= $uni;
  $name = "\@$name" if $name !~ /@/ && $name ne $s->{settings}->{friends_channel};

  my $channel = {name => $uni, visible_name => $name, no_modes => 1};
  $channel = new Irssi::Psyc::Channel($s->server, $channel, $auto||0);
  $s->{witem} = $channel;

  return 1;
}

sub insert_nicks {
  my ($s) = @_;
  return 0 unless $s->witem;

  for my $m (sort keys %{$s->{members}}) {
    my $nick = $s->{members}->{$m};
    $s->{members}->{$m} = $s->witem->nick_find($nick->{nick}) || $s->nick_insert($nick);
  }

  return 1;
}

sub nick_insert {
  my ($s, $nick) = @_;
  $s->debug('>> nick_insert:', $nick->{nick}, $nick->{host}, $s->witem->{name});
  my $n = $s->witem->nick_find($nick->{nick});
  return $n if $n;
  bless $nick, 'Irssi::Psyc::Nick';
  $nick = new Irssi::Psyc::Nick($s->server, $nick);
  $s->witem->nick_insert($nick);
  return $nick;
}

sub nick_change {
  my ($s, $old, $new) = @_;
  $s->debug('>> place nick change:', $old, $new, $s->{uni});
  return 0 unless $s->witem;
  if (my $nick = $s->witem->nick_find($old)) {
    $s->witem->nick_remove($nick);
    $s->nick_insert({nick => $new, host => $nick->{host}});
    $s->debug(">>> ok, place");
    return 1;
  }
  $s->debug(">>> fail, place");
  return 0;
}

sub find_uni_by_nick {
  my ($s, $nick) = @_;
  my $m = $s->{members};
  grep {return $_ if $m->{$_}->{nick} eq $nick} keys %$m;
  return '';
}

sub sync {
  my ($s) = @_;
  unless ($s->witem->{synced}) {
    $s->{witem} = $s->witem->update({joined => 1, synced => 1});
    Irssi::signal_emit("channel joined", $s->witem);
    Irssi::signal_emit("channel synced", $s->witem);
  }
}

1;
