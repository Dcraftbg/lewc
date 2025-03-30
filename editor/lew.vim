" Vim syntax file
" Language: Lew
" Version: 0.1.1

" Usage
" Put this in $VIMRUNTIME/syntax/lew.vim
" and add this to .vimrc:
" autocmd BufRead,BufNewFile *.lew set filetype=lew

if exists("b:current_syntax")
  finish
endif

set iskeyword=a-z,A-Z,_,48-57

let s:lew_preproc = ['c', 'import']
let s:lew_preproc_pattern     = '(' . join(s:lew_preproc, '|') . ')'

" Its a pretty dumb hack but it works I guess :D
execute 'syn match lewPreprocDirective /\v#\s*' . s:lew_preproc_pattern . '>/'
execute 'syn match lewPreprocError     /\v#\s*(' . s:lew_preproc_pattern . '>)@!\w*>/'
syn keyword lewKeywords         return extern if else while loop typedef struct null cast defer 
syn region  lewComment          start="//" keepend end="$"
syn keyword lewType             u8 u16 u32 u64 usize i8 i16 i32 i64 isize bool
" TODO: these large comments are incorrect as we match /*/**/*/ correctly
" inside the lexer but here the second */ gets left out
" syn region  lewComment          start="/\*" end="\*/"  
syn match   lewString           /\v(c)?".*"/
syn match   lewNumber           /\<\d[a-zA-Z0-9_]*/

highlight default link lewPreprocDirective PreProc
highlight default link lewPreprocError     Error
highlight default link lewNumber           Number
highlight default link lewKeywords         Keyword
highlight default link lewComment          Comment
highlight default link lewString           String
highlight default link lewType             Type 


let b:current_syntax = "lew"
