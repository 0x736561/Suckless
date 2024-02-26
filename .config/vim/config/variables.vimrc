" TOhtml "
let g:html_number_lines = 0
let g:mdslide_open_browser_cmd = "qutebrowser"

" não quebrar linhas automaticamente "
set nowrap
set nolinebreak
set textwidth=0 wrapmargin=0

" auto-completar código "
set omnifunc=syntaxcomplete#Complete

" suporte para mouse "
set mouse=a

" destacar linha selecionada "
set cursorline
:highlight Cursorline cterm=bold ctermbg=black

" destacar termo pesquisado "
set hlsearch

" pesquisar sem capitalização "
set ignorecase
set smartcase

" configurar espaçamento do tab "
set tabstop=4
set softtabstop=4
set shiftwidth=4
set textwidth=79
set expandtab
set autoindent

" Airline "
let g:airline_powerline_fonts = 1

" Netrw "
let g:netrw_liststyle = 3
let g:netrw_banner = 0
let g:netrw_browse_split = 4
let g:netrw_winsize = 20

" mostrar pares de (), {} e [] "
set showmatch

