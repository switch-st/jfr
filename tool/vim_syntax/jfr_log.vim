if exists("b:current_syntax")
  finish
endif

let s:cpo_save = &cpo
set cpo&vim

syn match   mBegin 		display '^' nextgroup=mIgnore1
syn match   mIgnore1 	contained display '\[' nextgroup=mDate
syn match   mDate 		contained display '\d\d\d\d-\d\d-\d\d' nextgroup=mIgnore2
syn match 	mIgnore2 	contained display '_' nextgroup=mTime
syn match   mTime 		contained display '\d\d:\d\d:\d\d' nextgroup=mIgnore3
syn match 	mIgnore3 	contained display '\] ' nextgroup=mModule
syn match 	mModule 	contained display '\w*' nextgroup=mIgnore4
syn match 	mIgnore4 	contained display '\[' nextgroup=mLevel
syn match 	mLevel 	 	contained display '\w*' nextgroup=mIgnore5  contains=lFATAL,lERROR,lWARNING,lINFO,lDEBUG
syn match 	mIgnore5 	contained display '\]: ' nextgroup=mMsg
syn match 	mMsg 	 	contained display 'w*' nextgroup=mIgnore6
syn match 	mIgnore6 	contained display '$'
syn match 	lFATAL 		'FATAL'
syn match 	lERROR 		'ERROR'
syn match 	lWARNING 	'WARNING'
syn match 	lINFO 		'INFO'
syn match 	lDEBUG 		'DEBUG\(_\d\)*'
syn match   messagesIP  '\d\+\.\d\+\.\d\+\.\d\+'
syn match   messagesURL '\w\+://\S\+'
syn match   messagesNumber      contained '0x[0-9a-fA-F]*\|\[<[0-9a-f]\+>\]\|\<\d[0-9a-fA-F]*'
syn match   messagesError       contained '\c.*\<\(FATAL\|ERROR\|ERRORS\|FAILED\|FAILURE\).*'


hi mDate term=standout cterm=bold ctermfg=1 guifg=Red
hi mTime term=standout cterm=bold ctermfg=1 guifg=Red
hi mModule term=standout cterm=bold ctermfg=4 guifg=#ff80ff
hi mLevel term=bold cterm=bold ctermfg=3 gui=bold guifg=Yellow
hi lFATAL term=standout cterm=bold ctermfg=7 ctermbg=1 guifg=White guibg=Red
hi lERROR term=standout cterm=bold ctermfg=1 guifg=Red
hi lWARNING term=bold cterm=bold ctermfg=3 gui=bold guifg=Yellow
hi lINFO term=standout cterm=bold ctermfg=2 gui=bold guifg=Green
hi lDEBUG term=bold cterm=bold gui=bold
hi def link messagesIP          Constant
hi def link messagesURL         Underlined
hi def link messagesNumber      Number
hi def link messagesError       ErrorMsg


let b:current_syntax = "jfr_log"

let &cpo = s:cpo_save
unlet s:cpo_save

