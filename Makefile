01-configure:
	doas setup-xorg-base
	doas apk add xorg-server-dev setxkbmap xprop xrandr \
		sx imlib2-dev libxft-dev pango-dev fontconfig-dev \
		xrdb libexif-dev freetype-dev patch gcc pkgconf \
		harfbuzz-dev libxcb-dev libx11-dev xcb-util-dev \
		cairo-dev ncurses g++ libxinerama-dev xf86-input-synaptics
	doas usermod -aG sys $USER
	doas usermod -aG adm $USER
	doas usermod -aG input $USER
	doas usermod -aG audio $USER
	doas usermod -aG video $USER
	doas mkdir /etc/X11/xorg.conf.d
	doas cp 99-synaptics-overrides.conf /etc/X11/xorg.conf.d/
02-programs:
	doas apk add vim cmus git zathura zathura-pdf-poppler \
		zathura-cb alsa-utils playerctl \
		brightnessctl pcmanfm font-jetbrains-mono \
		font-jetbrains-mono-nerd font-noto font-noto-cjk \
		font-noto-emoji font-noto-extra yt-dlp mpv \
		mpv-mpris xclip fzf exfatprogs curl wget htop \
		gsettings-desktop-schemas bash-completion \
		xdg-utils xdg-user-dirs
03-copy-config:
	cp -r .config ~/
	cp -r .local ~/
	cp .bash* ~/
04-clean:
	make clean --directory=dwm
	make clean --directory=st
	make clean --directory=dmenu
	make clean --directory=slstatus
	make clean --directory=bgs
	make clean --directory=slock
	make clean --directory=sent
	make clean --directory=smu
	make clean --directory=scroll
	make clean --directory=xscreenshot
	make clean --directory=nsxiv
	make clean --directory=noice
	make clean --directory=farbfeld
05-compile:
	make --directory=dwm
	make --directory=st
	make --directory=dmenu
	make --directory=slstatus
	make --directory=bgs
	make --directory=slock
	make --directory=sent
	make --directory=smu
	make --directory=scroll
	make --directory=xscreenshot
	make --directory=nsxiv
	make --directory=noice
	make --directory=farbfeld
06-install:
	doas make install --directory=dwm
	doas make install --directory=st
	doas make install --directory=dmenu
	doas make install --directory=slstatus
	doas make install --directory=bgs
	doas make install --directory=slock
	doas make install --directory=sent
	doas make install --directory=smu
	doas make install --directory=scroll
	doas make install --directory=xscreenshot
	doas make install --directory=nsxiv
	doas make install --directory=noice
	doas make install --directory=farbfeld


