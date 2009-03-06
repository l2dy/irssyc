irssi=${HOME}/.irssi
mode='u=rw,go=r'

all:
	make -C core
user-install:
	make -C core user-install
	find lib -name '*.pm' -exec install -m ${mode} -D {} ${irssi}/{} \;
	install -b -m ${mode} startup default.theme ${irssi}

install: user-install
