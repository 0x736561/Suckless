
PS1='[\033[32m\w\033[0m]\n~> '
bind '"\e[A":history-search-backward'
bind '"\e[B":history-search-forward'

# alias
alias grep='grep -i --color'
alias cp='cp -v'
alias mv='mv -v'
alias rm='rm -v'
alias mkdir='mkdir -p'
alias ll='ls -lah --group-directories-first'
alias l='ls -lh --group-directories-first'
alias ls='ls --color'
alias decrypt='gpg -dq ~/.authinfo.gpg >/dev/null'
alias mbsync='mbsync -c "$XDG_CONFIG_HOME"/isync/mbsyncrc'
alias post='ssg5 src/ dst/ "Sea" "https://0x736561.github.io"'
#alias vim='emacs -nw -bg black'

# git
alias config='/usr/bin/git --git-dir=$HOME/.dot --work-tree=$HOME'
