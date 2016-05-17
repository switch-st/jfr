" Add filetype for jfr_log
augroup filetypedetect
autocmd BufNewFile,BufRead *.log call Check_jfrlog()
autocmd BufNewFile,BufRead *.log_\d\d\d\d\d\d\d\d\d\d\d\d\d\d call Check_jfrlog()
autocmd BufNewFile,BufRead * call Check_jfrlog()
augroup END

func! Check_jfrlog()
	if getline(1) =~ '^\[\d\d\d\d-\d\d-\d\d_\d\d:\d\d:\d\d\] \w*\[\w*\]:.*$'
		setfiletype jfr_log
	endif
endfunc

