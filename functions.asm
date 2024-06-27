;Normalizer functions - Assembly Version
;By Jose Campos


.data
.code 

;Function to find average volume of block
;First input parameter (RCX): 16 bit signed array of samples (Address of array head)
;Second input parameter (RDX): 32 bit signed size of array
;Output parameter (RAX): 16 signed average value
average_vol PROC

	;Move address of short array from RCX to R12
	mov r12, rcx

	;Starting index stored in RBX
	mov rbx, 0

	;Move size of array (int) from EDX to ECX
	mov ecx, edx

	;Intial value for sum stored in R13
	mov r13, 0

	;Loop to add values
	Adding_loop:

	;Store current value in R14
	mov r14w, [r12 + rbx]
	movzx r14d, r14w

	;Test if absolute value is needed
	;Check sign of 16 bit portion of R14
	test r14w, r14w  

	;Skip if not negative or 0
    jns add_current

	;If negative, negate the 16 bit portion of R14
	neg r14w

	;Add current element to sum
	add_current:
	add r13d, r14d

	;Increase index
	add rbx, 2

	loop Adding_loop

	;Move sum into EAX as the dividend
	mov eax, r13d

	;Move divisor from EDX to R15D
	mov r15d, edx

	;Move 0 into EDX
	mov edx, 0

	;Divide EAX by EDX (sample size)
	idiv r15d

	ret
average_vol ENDP


;Function to find the reference volume for amplification
;First input parameter (RCX): 16 bit signed array of average values (Address of array head)
;Second input parameter (RDX): 32 bit signed size of array
;Output parameter (RAX): 16 signed reference value
ref_vol PROC

	;Move address of short array from RCX to R12
	mov r12, rcx

	;Starting index stored in RBX
	mov rbx, 0

	;Move size of array (int) from EDX to ECX
	mov ecx, edx

	;Intial value for max stored in RAX
	mov rax, 0

	;Loop to search for max average value
	Search_max:

	;Compare current max and current block average
	cmp ax, [r12 + rbx]

	;If current max is >= current block average, move to next value
	jge l_back

	;If current max < current block average, replace previous max
	mov ax, [r12 + rbx]
	
	l_back:
	;Increase index
	add rbx, 2

	loop Search_max
	
	ret
ref_vol ENDP

amp_factor PROC
	;Define SWORD type local variables
	LOCAL ref_val: SWORD
	LOCAL scal_fact: SWORD

	;Move address of short array from RCX to R12
	mov r12, rcx

	;Move size of array (int) from EDX to ECX
	mov ecx, edx

	;Starting index for short array stored in RBX
	mov rbx, 0

	;Starting index for int array stored in RDX
	mov rdx, 0

	;Move scalling factor (2^12) to scal_fact
	mov scal_fact, 4096

	;Intialize FPU
	finit

	;Move R8W into ref_val
	mov ref_val, r8w

	;Loop to compute amplification factor for 
	Compute_amp_fact:

	;Load ref_val into FPU
	fild ref_val

	;Divid ST(0) (ref_val) by current average value
	fidiv sword ptr [r12 + rbx]

	;Multiply by amplification factor
	fimul sword ptr scal_fact

	;Store in amplification factors array
	fistp sdword ptr [r9 + rdx]

	;Increase indexes
	add rbx, 2
	add rdx, 4

	loop Compute_amp_fact

	ret
amp_factor ENDP

apply_amp_fact PROC
	;Define SWORD type local variables
	LOCAL amp_fact: SDWORD
	LOCAL ampd_sample: SDWORD
	LOCAL scal_fact: SWORD
	
	;Move address of short array from RCX to R12
	mov r12, rcx

	;Move size of array (int) from EDX to ECX
	mov ecx, edx

	;Starting index for short array stored in RBX
	mov rbx, 0

	;Move r8d into amp_fact
	mov amp_fact, r8d

	;Move scalling factor (2^12) to scal_fact
	mov scal_fact, 4096

	;Intialize FPU
	finit

	;Loop to apply amplification factor to current sample
	amp_fact_loop:

	;Load amp_fact into FPU
	fild amp_fact

	;Multiply ST(0) (amp_fact) by current sample
	fimul sword ptr [r12 + rbx]

	;Divide result by scalling factor
	fidiv sword ptr scal_fact

	;Move max positive value into R13D
	mov r13d, 32767

	;Move amplified sample into ampd_sample
	fistp sdword ptr ampd_sample

	;Move ampd_sample into r14D
	mov r14d, ampd_sample

	;Compare amplified sample with max value
	cmp r13d, r14d

	;If max value > amplified sample, jump to compare of lower bound
	jg Compare_lower

	;If max value < amplified sample, over maximum positive value, store maximum instead
	;Store max value instead of overflowing sample
	mov [r12 + rbx], r13w

	;Loop back
	jmp loop_back

	;Check if amplified sample < -32768
	Compare_lower:
	
	;Move min negative value into R13D
	mov r13d, -32768

	;Compare amplified sample with min value
	cmp r13d, r14d

	;If min value < amplified sample, jump to store into current sample
	jl no_over_flow

	;If min value > amplified sample, over minimum negative value, store minimum instead
	;Store min value instead of overflowing sample
	mov [r12 + rbx], r13w

	;Loop back
	jmp loop_back

	no_over_flow:
	;Store amplified sample into current sample
	mov [r12 + rbx], r14w

	loop_back:

	;Increase index
	add rbx, 2

	loop amp_fact_loop

	ret
apply_amp_fact ENDP

END

