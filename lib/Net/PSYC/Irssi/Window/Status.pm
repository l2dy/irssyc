package Net::PSYC::Irssi::Window::Status;
use strict;
use warnings;
use base 'Net::PSYC::Irssi::Window';

sub new {
  my ($class, $irssi, $uni, $name) = @_;
  $class = ref $class || $class;
  my $s = Net::PSYC::Irssi::Window::new($class, $irssi, $uni, $name, 'status');
  bless $s, $class;
  return $s;
}

sub msg {
  my $s = shift;
  my ($uni, $name, $mc, $data, $vars, $tag) = @_;
  $s->SUPER::msg(@_, 1);
}

1;
