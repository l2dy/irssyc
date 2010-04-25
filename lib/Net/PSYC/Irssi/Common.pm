package Net::PSYC::Irssi::Common;
use strict;
use warnings;

use Irssi;
use Net::PSYC qw(parse_uniform make_uniform same_host);
use Data::Dumper;

sub store_one {
  my ($s, $o, $h, $k) = @_;
  #$s->debug('store_one:', $o, $h, $k);
  my $v = ref $o eq 'ARRAY' ? $o->[$k] : $o->{$k};
  my $ref = ref $v;
  my $n;
  if ($ref =~ /^Net::PSYC::Irssi/) {
    $n = $v->store;
  } elsif ($ref eq 'HASH' || $ref =~ /^Irssi::Psyc/) {
    $n = {};
    $s->store_one($v, $n, $_) for keys %$v;
  } elsif ($ref eq 'ARRAY') {
    $n = [];
    $s->store_one($v, $n, $_) for 0..@$v-1;
  } elsif (!$ref) {
    $n = $v;
  }
  ref $h eq 'ARRAY' ? $h->[$k] = $n : $h->{$k} = $n;
  return $h;
}

sub restore_one {
  my ($s, $o, $k) = @_;
  my $v = ref $o eq 'ARRAY' ? $o->[$k] : $o->{$k};
  my $ref = ref $v;
  if ($ref =~ /^Net::PSYC::Irssi/) {
    $v->restore($s);
  } elsif ($ref eq 'HASH' || $ref =~ /^Irssi::Psyc/) {
    $s->restore_one($v, $_) for keys %$v;
  } elsif ($ref eq 'ARRAY') {
    $s->restore_one($v, $_) for 0..@$v-1;
  }
}

sub get_source {
  my ($s, $vars, $c) = @_;
  my $uni;
  $s->debug('>> get_source:', $c);
  $uni = $vars->{_context} if $c;
  unless ($uni) {
    my $u = parse_uniform($vars->{_source});
    $s->debug('>>>', ref $u ? $u->{host} : '', 'vs', $s->{server_host});
    if (1 || ref $u && same_host($u->{host}, $s->{server_host})) {
      $s->debug('>>>> ok');
      $uni = $vars->{_source_relay};
    }
    $uni ||= $vars->{_source} || $vars->{_context};
  }
  $s->debug('>>> source:', $uni);
  return lc $uni;
}

sub get_uni {
  my ($s, $vars, $c) = @_;
  my $uni;
  if (ref $vars eq 'HASH') {
    $uni = $s->get_source($vars, $c);
  } else {
    $uni = $s->name2uni($vars);
  }
  return lc $uni;
}

sub get_name {
  my ($s, $vars, $uni, $c) = @_;
  $uni ||= $s->get_uni($vars, $c);
  return $s->{aliases}->{lc $uni} if exists $s->{aliases}->{lc $uni};
  $s->debug('>> get_name:', $vars, $uni, $c);
  my $u = parse_uniform($uni);
  $s->debug('>>>> get_name:', $uni) unless $u;
  return $uni if !ref $u || $u->{scheme} ne 'psyc';
  my $object = $u->{object};
  $object =~ s/^~//;
  my $nick; $nick = $vars->{_nick} if ref $vars eq 'HASH' && $vars->{_nick} && lc $object eq lc $vars->{_nick};
  if (same_host($u->{host}, $s->{server_host})) {
    $nick = $nick ? $nick : $object;
  } else {
    $u->{object} = "~$nick" if $nick;
    $nick = make_uniform($u->{user}, $u->{host}, $u->{port}, $u->{transport}, $u->{object});
  }
  $s->debug('>>>> get_name:', $nick);
  return $nick;
}

sub name2uni {
  my ($s, $name) = @_;
  $s->debug('>> name2uni:', $name);
  return $name unless $name;
  return $name if $name eq $s->{settings}->{friends_channel};
  return $s->{raliases}->{lc $name} if exists $s->{raliases}->{lc $name};
  if ($name !~ /^[a-z]+:/) {
    $name = "~$name" if $name !~ /^[~@]/;
    $name = $s->{server_uni}.$name;
  }
  $s->debug('>>>> name2uni:', $name);
  return lc $name;
}

sub place2uni {
  my ($s, $name) = @_;
  return $name unless $name;
  return $name if $name eq $s->{settings}->{friends_channel};
  if ($name !~ /^[a-z]+:/) {
    $name =~ s/^#/@/;
    $name = '@'.$name unless $s->is_place($name);
    $name = $s->{server_uni}.$name;
  }
  return lc $name;
}

sub legal_chars {
  return '\w_=+-';
}

sub legal_name {
  my ($s, $name) = @_;
  my $c = $s->legal_chars;
  return 1 if $name =~ /^[$c]+$/i;
}

sub is_place {
  my ($s, $name) = @_;
  my $c = $s->legal_chars;
  return 1 if $name =~ /^(\@|~[$c]+#)[$c]+$/i;
}

sub is_person {
  my ($s, $name) = @_;
  my $c = $s->legal_chars;
  return 1 if $name =~ /^~[$c]+$/i;
}

sub print {
  my $s = shift;
  my @p; push @p, $_||'' for (@_);
  Irssi::print "@p" if @p;
}

sub debug {
  my $s = shift;
  return unless $s->{settings}->{debug};
  my @p; push @p, $_||'' for (@_);
  print STDERR join(', ', @p), "\n";
}

sub dumper {
  my $s = shift;
  return unless $s->{settings}->{debug};
  $s->debug(Dumper(@_));
}

1;
