==54245== Memcheck, a memory error detector
==54245== Copyright (C) 2002-2017, and GNU GPL'd, by Julian Seward et al.
==54245== Using Valgrind-3.18.1 and LibVEX; rerun with -h for copyright info
==54245== Command: ./bin/memoria ./files/fibo.config
==54245== Parent PID: 54170
==54245== 
==54245== Thread 4:
==54245== Syscall param socketcall.sendto(msg) points to uninitialised byte(s)
==54245==    at 0x499C8FE: __libc_send (send.c:28)
==54245==    by 0x499C8FE: send (send.c:23)
==54245==    by 0x10EC65: enviar_paquete (serializacion.c:260)
==54245==    by 0x10B829: atender_solicitudes_kernel (memoria.c:221)
==54245==    by 0x4909AC2: start_thread (pthread_create.c:442)
==54245==    by 0x499AA03: clone (clone.S:100)
==54245==  Address 0x4e05152 is 114 bytes inside a block of size 136 alloc'd
==54245==    at 0x4848899: malloc (in /usr/libexec/valgrind/vgpreload_memcheck-amd64-linux.so)
==54245==    by 0x10EB7B: serializar_paquete (serializacion.c:242)
==54245==    by 0x10EC48: enviar_paquete (serializacion.c:258)
==54245==    by 0x10B829: atender_solicitudes_kernel (memoria.c:221)
==54245==    by 0x4909AC2: start_thread (pthread_create.c:442)
==54245==    by 0x499AA03: clone (clone.S:100)
==54245==  Uninitialised value was created by a heap allocation
==54245==    at 0x4848899: malloc (in /usr/libexec/valgrind/vgpreload_memcheck-amd64-linux.so)
==54245==    by 0x10AF5F: iniciar_memoria (memoria.c:51)
==54245==    by 0x10ADEA: main (memoria.c:17)
==54245== 
==54245== Thread 5:
==54245== Invalid read of size 8
==54245==    at 0x10BA63: crear_hilo (memoria.c:270)
==54245==    by 0x10B5AF: atender_solicitudes_kernel (memoria.c:171)
==54245==    by 0x4909AC2: start_thread (pthread_create.c:442)
==54245==    by 0x499AA03: clone (clone.S:100)
==54245==  Address 0x10 is not stack'd, malloc'd or (recently) free'd
==54245== 
==54245== 
==54245== Process terminating with default action of signal 11 (SIGSEGV)
==54245==  Access not within mapped region at address 0x10
==54245==    at 0x10BA63: crear_hilo (memoria.c:270)
==54245==    by 0x10B5AF: atender_solicitudes_kernel (memoria.c:171)
==54245==    by 0x4909AC2: start_thread (pthread_create.c:442)
==54245==    by 0x499AA03: clone (clone.S:100)
==54245==  If you believe this happened as a result of a stack
==54245==  overflow in your program's main thread (unlikely but
==54245==  possible), you can try to increase the size of the
==54245==  main thread stack using the --main-stacksize= flag.
==54245==  The main thread stack size used in this run was 8388608.
==54245== 
==54245== HEAP SUMMARY:
==54245==     in use at exit: 21,942 bytes in 1,336 blocks
==54245==   total heap usage: 140,537 allocs, 139,201 frees, 4,270,813 bytes allocated
==54245== 
==54245== Thread 1:
==54245== 2 bytes in 1 blocks are definitely lost in loss record 1 of 69
==54245==    at 0x484DCD3: realloc (in /usr/libexec/valgrind/vgpreload_memcheck-amd64-linux.so)
==54245==    by 0x486E64B: string_append (string.c:47)
==54245==    by 0x486F461: _string_append_with_format_list (string.c:312)
==54245==    by 0x486E83F: string_from_vformat (string.c:74)
==54245==    by 0x486E7D0: string_from_format (string.c:67)
==54245==    by 0x486E881: string_itoa (string.c:79)
==54245==    by 0x10B756: atender_solicitudes_kernel (memoria.c:210)
==54245==    by 0x4909AC2: start_thread (pthread_create.c:442)
==54245==    by 0x499AA03: clone (clone.S:100)
==54245== 
==54245== 2 bytes in 1 blocks are definitely lost in loss record 2 of 69
==54245==    at 0x484DCD3: realloc (in /usr/libexec/valgrind/vgpreload_memcheck-amd64-linux.so)
==54245==    by 0x486E64B: string_append (string.c:47)
==54245==    by 0x486F461: _string_append_with_format_list (string.c:312)
==54245==    by 0x486E83F: string_from_vformat (string.c:74)
==54245==    by 0x486E7D0: string_from_format (string.c:67)
==54245==    by 0x486E881: string_itoa (string.c:79)
==54245==    by 0x10B763: atender_solicitudes_kernel (memoria.c:210)
==54245==    by 0x4909AC2: start_thread (pthread_create.c:442)
==54245==    by 0x499AA03: clone (clone.S:100)
==54245== 
==54245== 2 bytes in 1 blocks are definitely lost in loss record 3 of 69
==54245==    at 0x484DCD3: realloc (in /usr/libexec/valgrind/vgpreload_memcheck-amd64-linux.so)
==54245==    by 0x486E64B: string_append (string.c:47)
==54245==    by 0x486F461: _string_append_with_format_list (string.c:312)
==54245==    by 0x486E83F: string_from_vformat (string.c:74)
==54245==    by 0x486E7D0: string_from_format (string.c:67)
==54245==    by 0x486E881: string_itoa (string.c:79)
==54245==    by 0x10D9E4: obtener_proceso_activo (memoria.c:949)
==54245==    by 0x10DFB5: liberar_hueco_dinamico (memoria.c:1084)
==54245==    by 0x10DF33: liberar_espacio_en_memoria (memoria.c:1067)
==54245==    by 0x10BAC6: finalizar_proceso (memoria.c:280)
==54245==    by 0x10B650: atender_solicitudes_kernel (memoria.c:184)
==54245==    by 0x4909AC2: start_thread (pthread_create.c:442)
==54245== 
==54245== 7 bytes in 1 blocks are definitely lost in loss record 9 of 69
==54245==    at 0x4848899: malloc (in /usr/libexec/valgrind/vgpreload_memcheck-amd64-linux.so)
==54245==    by 0x10E996: buffer_read_string (serializacion.c:100)
==54245==    by 0x10EF28: recibir_mensaje (serializacion.c:351)
==54245==    by 0x10B861: atender_solicitudes_kernel (memoria.c:225)
==54245==    by 0x4909AC2: start_thread (pthread_create.c:442)
==54245==    by 0x499AA03: clone (clone.S:100)
==54245== 
==54245== 13 bytes in 1 blocks are definitely lost in loss record 18 of 69
==54245==    at 0x4848899: malloc (in /usr/libexec/valgrind/vgpreload_memcheck-amd64-linux.so)
==54245==    by 0x491D58E: strdup (strdup.c:42)
==54245==    by 0x486D87B: temporal_get_string_time (temporal.c:27)
==54245==    by 0x10B70D: atender_solicitudes_kernel (memoria.c:205)
==54245==    by 0x4909AC2: start_thread (pthread_create.c:442)
==54245==    by 0x499AA03: clone (clone.S:100)
==54245== 
==54245== 14 bytes in 14 blocks are definitely lost in loss record 19 of 69
==54245==    at 0x4848899: malloc (in /usr/libexec/valgrind/vgpreload_memcheck-amd64-linux.so)
==54245==    by 0x491D58E: strdup (strdup.c:42)
==54245==    by 0x486E5FD: string_duplicate (string.c:43)
==54245==    by 0x486E719: string_new (string.c:60)
==54245==    by 0x10B44F: atender_solicitudes_kernel (memoria.c:142)
==54245==    by 0x4909AC2: start_thread (pthread_create.c:442)
==54245==    by 0x499AA03: clone (clone.S:100)
==54245== 
==54245== 16 bytes in 1 blocks are definitely lost in loss record 27 of 69
==54245==    at 0x4848899: malloc (in /usr/libexec/valgrind/vgpreload_memcheck-amd64-linux.so)
==54245==    by 0x10D7E7: asignar_particion_dinamica (memoria.c:906)
==54245==    by 0x10D6EC: particion_dinamica (memoria.c:876)
==54245==    by 0x10CF5C: hay_particion_disponible (memoria.c:722)
==54245==    by 0x10B958: crear_proceso (memoria.c:247)
==54245==    by 0x10B4CD: atender_solicitudes_kernel (memoria.c:154)
==54245==    by 0x4909AC2: start_thread (pthread_create.c:442)
==54245==    by 0x499AA03: clone (clone.S:100)
==54245== 
==54245== 96 bytes in 1 blocks are definitely lost in loss record 42 of 69
==54245==    at 0x4848899: malloc (in /usr/libexec/valgrind/vgpreload_memcheck-amd64-linux.so)
==54245==    by 0x10CA52: leer_de_memoria (memoria.c:605)
==54245==    by 0x10B748: atender_solicitudes_kernel (memoria.c:209)
==54245==    by 0x4909AC2: start_thread (pthread_create.c:442)
==54245==    by 0x499AA03: clone (clone.S:100)
==54245== 
==54245== 160 (16 direct, 144 indirect) bytes in 1 blocks are definitely lost in loss record 48 of 69
==54245==    at 0x4848899: malloc (in /usr/libexec/valgrind/vgpreload_memcheck-amd64-linux.so)
==54245==    by 0x10EA75: crear_paquete (serializacion.c:224)
==54245==    by 0x10B792: atender_solicitudes_kernel (memoria.c:212)
==54245==    by 0x4909AC2: start_thread (pthread_create.c:442)
==54245==    by 0x499AA03: clone (clone.S:100)
==54245== 
==54245== 272 bytes in 1 blocks are possibly lost in loss record 53 of 69
==54245==    at 0x484DA83: calloc (in /usr/libexec/valgrind/vgpreload_memcheck-amd64-linux.so)
==54245==    by 0x40147D9: calloc (rtld-malloc.h:44)
==54245==    by 0x40147D9: allocate_dtv (dl-tls.c:375)
==54245==    by 0x40147D9: _dl_allocate_tls (dl-tls.c:634)
==54245==    by 0x490A7B4: allocate_stack (allocatestack.c:430)
==54245==    by 0x490A7B4: pthread_create@@GLIBC_2.34 (pthread_create.c:647)
==54245==    by 0x10B2C0: atender_solicitudes (memoria.c:103)
==54245==    by 0x10AEBF: main (memoria.c:32)
==54245== 
==54245== 272 bytes in 1 blocks are possibly lost in loss record 54 of 69
==54245==    at 0x484DA83: calloc (in /usr/libexec/valgrind/vgpreload_memcheck-amd64-linux.so)
==54245==    by 0x40147D9: calloc (rtld-malloc.h:44)
==54245==    by 0x40147D9: allocate_dtv (dl-tls.c:375)
==54245==    by 0x40147D9: _dl_allocate_tls (dl-tls.c:634)
==54245==    by 0x490A7B4: allocate_stack (allocatestack.c:430)
==54245==    by 0x490A7B4: pthread_create@@GLIBC_2.34 (pthread_create.c:647)
==54245==    by 0x10B2F2: atender_solicitudes (memoria.c:106)
==54245==    by 0x10AEBF: main (memoria.c:32)
==54245== 
==54245== 327 bytes in 327 blocks are definitely lost in loss record 57 of 69
==54245==    at 0x4848899: malloc (in /usr/libexec/valgrind/vgpreload_memcheck-amd64-linux.so)
==54245==    by 0x491D58E: strdup (strdup.c:42)
==54245==    by 0x486E5FD: string_duplicate (string.c:43)
==54245==    by 0x486E719: string_new (string.c:60)
==54245==    by 0x10CB13: buscar_instruccion (memoria.c:620)
==54245==    by 0x10BDA9: atender_cpu (memoria.c:354)
==54245==    by 0x4909AC2: start_thread (pthread_create.c:442)
==54245==    by 0x499AA03: clone (clone.S:100)
==54245== 
==54245== 544 bytes in 2 blocks are possibly lost in loss record 61 of 69
==54245==    at 0x484DA83: calloc (in /usr/libexec/valgrind/vgpreload_memcheck-amd64-linux.so)
==54245==    by 0x40147D9: calloc (rtld-malloc.h:44)
==54245==    by 0x40147D9: allocate_dtv (dl-tls.c:375)
==54245==    by 0x40147D9: _dl_allocate_tls (dl-tls.c:634)
==54245==    by 0x490A7B4: allocate_stack (allocatestack.c:430)
==54245==    by 0x490A7B4: pthread_create@@GLIBC_2.34 (pthread_create.c:647)
==54245==    by 0x10B3EF: atender_kernel (memoria.c:131)
==54245==    by 0x4909AC2: start_thread (pthread_create.c:442)
==54245==    by 0x499AA03: clone (clone.S:100)
==54245== 
==54245== 684 bytes in 327 blocks are definitely lost in loss record 62 of 69
==54245==    at 0x484DCD3: realloc (in /usr/libexec/valgrind/vgpreload_memcheck-amd64-linux.so)
==54245==    by 0x486E64B: string_append (string.c:47)
==54245==    by 0x486F461: _string_append_with_format_list (string.c:312)
==54245==    by 0x486E83F: string_from_vformat (string.c:74)
==54245==    by 0x486E7D0: string_from_format (string.c:67)
==54245==    by 0x486E881: string_itoa (string.c:79)
==54245==    by 0x10CAD2: buscar_instruccion (memoria.c:617)
==54245==    by 0x10BDA9: atender_cpu (memoria.c:354)
==54245==    by 0x4909AC2: start_thread (pthread_create.c:442)
==54245==    by 0x499AA03: clone (clone.S:100)
==54245== 
==54245== LEAK SUMMARY:
==54245==    definitely lost: 1,179 bytes in 676 blocks
==54245==    indirectly lost: 144 bytes in 2 blocks
==54245==      possibly lost: 1,088 bytes in 4 blocks
==54245==    still reachable: 19,531 bytes in 654 blocks
==54245==         suppressed: 0 bytes in 0 blocks
==54245== Reachable blocks (those to which a pointer was found) are not shown.
==54245== To see them, rerun with: --leak-check=full --show-leak-kinds=all
==54245== 
==54245== For lists of detected and suppressed errors, rerun with: -s
==54245== ERROR SUMMARY: 16 errors from 16 contexts (suppressed: 0 from 0)
