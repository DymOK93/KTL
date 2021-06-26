; Licensed under the MIT License <http://opensource.org/licenses/MIT>.
; Copyright (c) Dmitry Bolshakov aka Dym93OK

extern report_security_check_failure: proc
extern __security_cookie: qword

.code

__security_check_cookie proc public
	cmp rcx, __security_cookie
	bnd jnz short ReportFailure
	rol rcx, 10h
	test cx, 0FFFFh
	bnd jnz short RestoreRcx
	ret

RestoreRcx:                            
	ror rcx, 10h

ReportFailure:
	jmp report_security_check_failure 

__security_check_cookie endp

end

