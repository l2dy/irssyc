#!/bin/sh

base=`pwd`
irssi="$HOME/.irssi"
mode='u=rw,go=r'

echo 'This will make and install all prerequisites for irssi-psyc'
echo -n 'continue? [Y/n] '
read cont
if !([ "$cont" == "y" ] || [ "$cont" == "yes" ] || [ "$cont" == "" ]); then exit 1; fi

echo -n 'download sources? [Y/n] '
read cont
if ([ "$cont" == "y" ] || [ "$cont" == "yes" ] || [ "$cont" == "" ]); then
  wget -c http://irssi.org/files/irssi-0.8.12.tar.gz http://perl.psyc.eu/files/perlPSYC-0.26.zip
fi
echo -n 'unpack sources? [Y/n] '
read cont
if ([ "$cont" == "y" ] || [ "$cont" == "yes" ] || [ "$cont" == "" ]); then
  tar zxvf irssi-0.8.12.tar.gz
  unzip perlPSYC-0.26.zip
fi
echo -n 'patch sources? [Y/n] '
read cont
if ([ "$cont" == "y" ] || [ "$cont" == "yes" ] || [ "$cont" == "" ]); then
  patch -p0 -d irssi-0.8.12 <irssi-0.8.12-tg.patch
  #patch -p0 -d perlpsyc <perlPSYC-0.25-tg.patch
fi
echo -n "install perlPSYC to $irssi/lib? [Y/n] "
read cont
if ([ "$cont" == "y" ] || [ "$cont" == "yes" ] || [ "$cont" == "" ]); then
  cd $base/perlpsyc/lib/perlxt/Net
  find . -name '*.pm' -exec install -m $mode -D {} $irssi/lib/Net/{} \;
fi

echo -n 'make irssi? [Y/n] '
read cont
if ([ "$cont" == "y" ] || [ "$cont" == "yes" ] || [ "$cont" == "" ]); then
  cd $base/irssi-0.8.12
  ./configure --with-proxy --with-bot --with-socks --enable-ssl --enable-ipv6 --with-gc
  make

  echo -n 'sudo make install irssi? [Y/n] '
  read cont
  if ([ "$cont" == "y" ] || [ "$cont" == "yes" ] || [ "$cont" == "" ]); then
    sudo make install
  fi
fi

echo "Done. Now you can continue with make all install"