package Net::PSYC::Irssi::Window::Friends;
use strict;
use warnings;
use base 'Net::PSYC::Irssi::Window::Channel';

use Net::PSYC qw(psyctext);
use Irssi;
use Irssi::TextUI;

sub new {
  my ($class, $irssi, $uni, $name) = @_;
  my $s = Net::PSYC::Irssi::Window->new($irssi, $uni, $name, 'friends');
  $s->{members} ||= {$s->server->{nick} => {'nick' => $s->server->{nick}, 'host' => $s->{irssi}->{uni}, op => 1}};
  $s->{sb} = {here => 0, away => 0};
  $s->{sb_here} = [];
  $s->{sb_away} = [];
  $s->{sb_here_old} = [];
  $s->{sb_away_old} = [];
  bless $s, $class;
  $s->debug('#### NEW FRIENDS');
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

sub msg {
  my $s = shift;
  my ($uni, $name, $mc, $data, $vars, $tag) = @_;
  my $print = 0;
  $_ = $mc;
 SWITCH: {
    /^_list_acquaintance_each/ && do {
      $s->channel_create($s->{uni}, $s->{name}, 1) unless $s->witem;
      my $host = $s->get_uni($vars->{_acquaintance});
      my $nick = $s->get_name($host);
      $_ = $vars->{_type_notification};
      if (/^_pending/) {
        $s->print_notice("Pending: $host");
      } elsif (/^_offered/) {
        $s->print_notice("Offered: $host");
      } else {
        $s->{members}->{$host} = {nick => $nick, host => $host};
        $s->insert_nicks;
      }
      last SWITCH;
    };
    /^_list_friends_(present|away)/ && do {
      my $here = $1 eq 'present' ? 1 : 0;
      my $away = $1 eq 'away' ? 1 : 0;
      for my $n (@{$vars->{_list_friends}}) {
        #$n = $s->get_name($n);
        my $nick = $s->{members}->{$n};
        $s->{members}->{$n} = $s->update_presence($nick, $here, $away) if $nick;
      }
      $s->sync;

      $s->{sb_here} = [sort split ', ', $vars->{_friends}] if $here;
      $s->{sb_away} = [sort split ', ', $vars->{_friends_away}] if $away;

      $s->update_statusbars;
      last SWITCH;
    };
    /^(_request_friendship_implied|_notice_friendship_established)/ && do {
      my $nick = $vars->{_nick_target} || $name;
      if ($s->{members}->{$uni}) {
        $s->print_notice("Friendship request: $nick is already your friend");
        return 0;
      }
      $s->{members}->{$uni} = $s->nick_insert({nick => $nick, host => $uni});
      my $msg = 'is your friend now';
      $s->print_format(MSGLEVEL_JOINS, 'join_msg', $nick, $uni, $msg);
      last SWITCH;
    };
    /^(_echo_friendship_removed|_notice_friendship_removed)/ && do {
      my $nick = $vars->{_nick_target} || $name;
      unless ($s->{members}->{$uni}) {
        $s->print_notice("Friendship remove: $nick is not you friend");
        return 0;
      }
      $s->witem->nick_remove($s->{members}->{$uni});
      delete $s->{members}->{$uni};

      my $msg = 'is not your friend anymore';
      $msg = psyctext('deletes [_possessive] friendship with you', $vars) if $mc =~ /^_notice_friendship_removed/;
      $s->print_format(MSGLEVEL_PARTS, 'part_msg', $nick, $uni, $msg);
      last SWITCH;
    };
    /^_notice_presence(_here|_away)?/ && do {
      my $nick = $s->{members}->{$uni};
      unless ($nick) {
        $s->debug('!!!!! z0mg no friend nick !!!!!');
        return 0;
      }

      my $av = $vars->{_degree_availability};
      my ($here, $away) = 0;
      $away = 1 if $av >= 3 && $av <= 6;
      $here = 1 if $av >= 7;
      $s->{members}->{$uni} = $s->update_presence($nick, $here, $away);

      unless ($data) {
        $data = '[_nick] is absent now. [_description_presence]';
        $data = '[_nick] is busy now. [_description_presence]' if $away;
        $data = '[_nick] is available now. [_description_presence]' if $here;
      }
      $data =~ s/$/ ([_degree_availability])/;

      $s->irssi->command(undef, 'friends');
      $print = 1;
      last SWITCH;
    };
    $print = 1;
  }

  $s->notice(@_) if $print;
  $s->SUPER::msg(@_);
}

sub witem_created {
  my ($s, $witem) = @_;
  $s->debug('>> friends witem created');
  $s->{witem} = $witem;
  $s->insert_nicks;
}

sub notice {
  my ($s, $uni, $name, $mc, $data, $vars, $nick) = @_;
  $vars->{_nick} = $nick if $nick;
  $s->print_notice(psyctext($data, $vars));

  if (my $win = $s->server->window_item_find($uni)) {
    $data =~ s/[\r\n].*//;
    my $text = psyctext($data, $vars);
    #$text = $s->recode->convert($text) || $text if $s->recode;
    $text = Irssi::recode_in($s->server, $text, $s->{uni});
    $win->print($text, MSGLEVEL_SNOTES);
  }
}

sub update_presence {
  my ($s, $nick, $here, $away) = @_;
  $s->debug('>> update presence:', $nick, $here, $away);
  return $nick->update({gone => $here ? 0 : 1, halfop => $here ? 1 : 0, voice => $here || $away ? 1 : 0, last_check => time});
}

sub update_statusbars {
  my ($s) = @_;
  $s->update_statusbar('here', $s->{sb_here}) if "$s->{sb_here}" ne "$s->{sb_here_old}";
  $s->update_statusbar('away', $s->{sb_away}) if "$s->{sb_away}" ne "$s->{sb_away_old}";

  $s->{sb_here_old} = $s->{sb_here};
  $s->{sb_away_old} = $s->{sb_away};
}

sub update_statusbar {
  my ($s, $type, $sb) = (@_);
  my $name = "psyc_friends_$type";
  $s->remove_statusbars($type);

  my $w = 6;
  for (@$sb) {
    $w += length "$_, ";
    #$s->debug("$w: $_,");
    if (!$s->{sb}->{$type} || $w-1 > $s->{settings}->{terminal}->{width}) {
      #$s->debug('new');
      $w = 6 + length "$_, " if $s->{sb}->{$type};
      $name = $s->create_statusbar($type);
    }
    #$s->debug(">> statusbar $name add '$_'");
    Irssi::statusbar_item_register($_, "$_, ");
    Irssi::command("statusbar $name add '$_'");
  }

  my $act = Irssi::active_win->{active};
  if ($act && $act->{name} eq $s->{uni}) {
    for (my $i=0; $i<$s->{sb}->{$type}; $i++) {
      Irssi::command("statusbar psyc_friends_${type}_$i visible active");
    }
  }

  Irssi::statusbar_items_redraw($name);
}

sub create_statusbar {
  my ($s, $type) = @_;
  my $n = $s->{sb}->{$type}++;
  my $name = "psyc_friends_${type}_$n";
  $s->debug(">> statusbar $name add");

  my $str = '      ';
  $str = $type eq 'here' ? 'Here: ' : 'Away: ' unless $n;
  my $x = $type eq 'here' ? 0 : 10;

  Irssi::statusbar_item_register($name, $str, '');
  Irssi::statusbars_recreate_items();

  Irssi::command("statusbar $name reset");
  Irssi::command("statusbar $name type window");
  Irssi::command("statusbar $name placement top");
  Irssi::command("statusbar $name position ".($x+$n));
  Irssi::command("statusbar $name visible inactive");
  Irssi::command("statusbar $name add $name");
  Irssi::command("statusbar $name enable");

  return $name;
}

sub remove_statusbar {
  my ($s, $type) = @_;
  my $n = --$s->{sb}->{$type};
  my $name = "psyc_friends_${type}_$n";
  $s->debug(">> statusbar $name reset");
  Irssi::command("statusbar $name reset");
}

sub remove_statusbars {
  my ($s, $type) = @_;
  $s->remove_statusbar($type, $_) while $s->{sb}->{$type};
}

sub window_changed_to {
  my ($s) = @_;
  $s->debug("window changed to friends");
  Irssi::command('statusbar topic disable');
  Irssi::command("statusbar psyc_friends_here_$_ visible active") for 0..$s->{sb}->{here}-1;
  Irssi::command("statusbar psyc_friends_away_$_ visible active") for 0..$s->{sb}->{away}-1;
}

sub window_changed_away {
  my ($s) = @_;
  $s->debug("window changed away from friends");
  Irssi::command("statusbar psyc_friends_here_$_ visible inactive") for 0..$s->{sb}->{here}-1;
  Irssi::command("statusbar psyc_friends_away_$_ visible inactive") for 0..$s->{sb}->{away}-1;
  Irssi::command('statusbar topic enable');
}

1;
