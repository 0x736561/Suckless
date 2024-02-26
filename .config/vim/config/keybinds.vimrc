" leader "
let mapleader = " "
let timeoutlen=1000

" buffers "
map <leader>. <Esc>:bn<cr>
map <leader>, <Esc>:bp<cr>
map <leader>d <Esc>:bdel<cr>

" splits  "
map <leader>v <Esc>:vs<cr>
map <leader>m <Esc>:sp<cr>

" movimentação entre janelas "
map <leader>h <c-w>h
map <leader>j <c-w>j
map <leader>k <c-w>k
map <leader>l <c-w>l

" FuzzyFinder "
map <leader>f <Esc>:Files<cr>
map <leader>b <Esc>:Buffers<cr>

" Netrw  "
map <leader>n <Esc>:Vexplore<cr>

" terminal "
map <leader>t <Esc>:terminal<cr><c-w><c-r>

" recarregar config "
map <leader>r <Esc>:so $XDG_CONFIG_HOME/vim/vimrc<cr><Esc>:PlugInstall<cr>

" temas  "
map <leader>cd <Esc>:colorscheme dracula<cr><Esc>:silent exec "!echo 'colorscheme dracula' > ~/.config/vim/config/color.vimrc"<cr><c-l>
map <leader>ce <Esc>:colorscheme everforest<cr><Esc>:silent exec "!echo 'colorscheme everforest' > ~/.config/vim/config/color.vimrc"<cr><c-l>
map <leader>cg <Esc>:colorscheme gruvbox<cr><Esc>:silent exec "!echo 'colorscheme gruvbox' > ~/.config/vim/config/color.vimrc"<cr><c-l>
map <leader>cm <Esc>:colorscheme monokai<cr><Esc>:silent exec "!echo 'colorscheme monokai' > ~/.config/vim/config/color.vimrc"<cr><c-l>
map <leader>cn <Esc>:colorscheme nord<cr><Esc>:silent exec "!echo 'colorscheme nord' > ~/.config/vim/config/color.vimrc"<cr><c-l>
map <leader>co <Esc>:colorscheme onedark<cr><Esc>:silent exec "!echo 'colorscheme onedark' > ~/.config/vim/config/color.vimrc"<cr><c-l>
map <leader>cs <Esc>:colorscheme srcery<cr><Esc>:silent exec "!echo 'colorscheme srcery' > ~/.config/vim/config/color.vimrc"<cr><c-l>

" etc "
map <leader>q <Esc>:wincmd q<cr>
map <leader>s <Esc>:w<cr>
map <leader>iv <Esc>:version<cr>
