export PATH=$HOME/.local/bin:$PATH
export XDG_DATA_HOME=$HOME/.local/share
export XDG_CONFIG_HOME=$HOME/.config
export XDG_STATE_HOME=$HOME/.local/state
export XDG_CACHE_HOME=$HOME/.cache
export HISTFILE="$XDG_DATA_HOME"/bash/history
export GNUPGHOME="$XDG_DATA_HOME"/gnupg
export ICEAUTHORITY="$XDG_CACHE_HOME"/ICEauthority
export PASSWORD_STORE_DIR="$XDG_DATA_HOME"/pass
export ANDROID_HOME="$XDG_DATA_HOME"/android
export XDG_CURRENT_DESKTOP=sway
export CARGO_HOME="$XDG_DATA_HOME"/cargo
export GVIMINIT='let $MYGVIMRC="$XDG_CONFIG_HOME/vim/gvimrc" | source $MYGVIMRC'
export VIMINIT='let $MYVIMRC="$XDG_CONFIG_HOME/vim/vimrc" | source $MYVIMRC'
export EDITOR='vim'


if [ -f ~/.bashrc ]; then
	. ~/.bashrc
fi

if [[ -z "$DISPLAY" ]] && [[ $(tty) = /dev/tty1 ]]; then
	sx
fi
