
wrapper.o:     file format elf32-tradbigmips

Disassembly of section .text:

00000000 <sesc_reg_com>:
   0:	3c1c0000 	lui	gp,0x0
   4:	279c0000 	addiu	gp,gp,0
   8:	0399e021 	addu	gp,gp,t9
   c:	27bdff98 	addiu	sp,sp,-104
  10:	afbc0000 	sw	gp,0(sp)
  14:	afbf0030 	sw	ra,48(sp)
  18:	afbe002c 	sw	s8,44(sp)
  1c:	afbc0028 	sw	gp,40(sp)
  20:	afb70024 	sw	s7,36(sp)
  24:	afb60020 	sw	s6,32(sp)
  28:	afb5001c 	sw	s5,28(sp)
  2c:	afb40018 	sw	s4,24(sp)
  30:	afb30014 	sw	s3,20(sp)
  34:	afb20010 	sw	s2,16(sp)
  38:	afb1000c 	sw	s1,12(sp)
  3c:	afb00008 	sw	s0,8(sp)
  40:	e7bf0060 	swc1	$f31,96(sp)
  44:	e7be0064 	swc1	$f30,100(sp)
  48:	e7bd0058 	swc1	$f29,88(sp)
  4c:	e7bc005c 	swc1	$f28,92(sp)
  50:	e7bb0050 	swc1	$f27,80(sp)
  54:	e7ba0054 	swc1	$f26,84(sp)
  58:	e7b90048 	swc1	$f25,72(sp)
  5c:	e7b8004c 	swc1	$f24,76(sp)
  60:	e7b70040 	swc1	$f23,64(sp)
  64:	e7b60044 	swc1	$f22,68(sp)
  68:	e7b50038 	swc1	$f21,56(sp)
  6c:	e7b4003c 	swc1	$f20,60(sp)
  70:	03a0f021 	move	s8,sp
  74:	00000000 	nop
  78:	03c0e821 	move	sp,s8
  7c:	8fbf0030 	lw	ra,48(sp)
  80:	8fbe002c 	lw	s8,44(sp)
  84:	8fb70024 	lw	s7,36(sp)
  88:	8fb60020 	lw	s6,32(sp)
  8c:	8fb5001c 	lw	s5,28(sp)
  90:	8fb40018 	lw	s4,24(sp)
  94:	8fb30014 	lw	s3,20(sp)
  98:	8fb20010 	lw	s2,16(sp)
  9c:	8fb1000c 	lw	s1,12(sp)
  a0:	8fb00008 	lw	s0,8(sp)
  a4:	c7bf0060 	lwc1	$f31,96(sp)
  a8:	c7be0064 	lwc1	$f30,100(sp)
  ac:	c7bd0058 	lwc1	$f29,88(sp)
  b0:	c7bc005c 	lwc1	$f28,92(sp)
  b4:	c7bb0050 	lwc1	$f27,80(sp)
  b8:	c7ba0054 	lwc1	$f26,84(sp)
  bc:	c7b90048 	lwc1	$f25,72(sp)
  c0:	c7b8004c 	lwc1	$f24,76(sp)
  c4:	c7b70040 	lwc1	$f23,64(sp)
  c8:	c7b60044 	lwc1	$f22,68(sp)
  cc:	c7b50038 	lwc1	$f21,56(sp)
  d0:	c7b4003c 	lwc1	$f20,60(sp)
  d4:	03e00008 	jr	ra
  d8:	27bd0068 	addiu	sp,sp,104

000000dc <w_sesc_begin_versioning_>:
  dc:	3c1c0000 	lui	gp,0x0
  e0:	279c0000 	addiu	gp,gp,0
  e4:	0399e021 	addu	gp,gp,t9
  e8:	27bdffd8 	addiu	sp,sp,-40
  ec:	afbc0010 	sw	gp,16(sp)
  f0:	afbf0020 	sw	ra,32(sp)
  f4:	afbe001c 	sw	s8,28(sp)
  f8:	afbc0018 	sw	gp,24(sp)
  fc:	03a0f021 	move	s8,sp
 100:	8f990000 	lw	t9,0(gp)
 104:	00000000 	nop
 108:	0320f809 	jalr	t9
 10c:	00000000 	nop
 110:	8fdc0010 	lw	gp,16(s8)
 114:	03c0e821 	move	sp,s8
 118:	8fbf0020 	lw	ra,32(sp)
 11c:	8fbe001c 	lw	s8,28(sp)
 120:	03e00008 	jr	ra
 124:	27bd0028 	addiu	sp,sp,40

00000128 <w_sesc_fork_successor_>:
 128:	3c1c0000 	lui	gp,0x0
 12c:	279c0000 	addiu	gp,gp,0
 130:	0399e021 	addu	gp,gp,t9
 134:	27bdffd8 	addiu	sp,sp,-40
 138:	afbc0010 	sw	gp,16(sp)
 13c:	afbf0020 	sw	ra,32(sp)
 140:	afbe001c 	sw	s8,28(sp)
 144:	afbc0018 	sw	gp,24(sp)
 148:	03a0f021 	move	s8,sp
 14c:	8f990000 	lw	t9,0(gp)
 150:	00000000 	nop
 154:	0320f809 	jalr	t9
 158:	00000000 	nop
 15c:	8fdc0010 	lw	gp,16(s8)
 160:	03c0e821 	move	sp,s8
 164:	8fbf0020 	lw	ra,32(sp)
 168:	8fbe001c 	lw	s8,28(sp)
 16c:	03e00008 	jr	ra
 170:	27bd0028 	addiu	sp,sp,40

00000174 <w_sesc_become_safe_>:
 174:	3c1c0000 	lui	gp,0x0
 178:	279c0000 	addiu	gp,gp,0
 17c:	0399e021 	addu	gp,gp,t9
 180:	27bdffd8 	addiu	sp,sp,-40
 184:	afbc0010 	sw	gp,16(sp)
 188:	afbf0020 	sw	ra,32(sp)
 18c:	afbe001c 	sw	s8,28(sp)
 190:	afbc0018 	sw	gp,24(sp)
 194:	03a0f021 	move	s8,sp
 198:	8f990000 	lw	t9,0(gp)
 19c:	00000000 	nop
 1a0:	0320f809 	jalr	t9
 1a4:	00000000 	nop
 1a8:	8fdc0010 	lw	gp,16(s8)
 1ac:	03c0e821 	move	sp,s8
 1b0:	8fbf0020 	lw	ra,32(sp)
 1b4:	8fbe001c 	lw	s8,28(sp)
 1b8:	03e00008 	jr	ra
 1bc:	27bd0028 	addiu	sp,sp,40

000001c0 <w_sesc_end_versioning_>:
 1c0:	3c1c0000 	lui	gp,0x0
 1c4:	279c0000 	addiu	gp,gp,0
 1c8:	0399e021 	addu	gp,gp,t9
 1cc:	27bdffd8 	addiu	sp,sp,-40
 1d0:	afbc0010 	sw	gp,16(sp)
 1d4:	afbf0020 	sw	ra,32(sp)
 1d8:	afbe001c 	sw	s8,28(sp)
 1dc:	afbc0018 	sw	gp,24(sp)
 1e0:	03a0f021 	move	s8,sp
 1e4:	8f990000 	lw	t9,0(gp)
 1e8:	00000000 	nop
 1ec:	0320f809 	jalr	t9
 1f0:	00000000 	nop
 1f4:	8fdc0010 	lw	gp,16(s8)
 1f8:	03c0e821 	move	sp,s8
 1fc:	8fbf0020 	lw	ra,32(sp)
 200:	8fbe001c 	lw	s8,28(sp)
 204:	03e00008 	jr	ra
 208:	27bd0028 	addiu	sp,sp,40

0000020c <w_sesc_is_safe_>:
 20c:	3c1c0000 	lui	gp,0x0
 210:	279c0000 	addiu	gp,gp,0
 214:	0399e021 	addu	gp,gp,t9
 218:	27bdffd8 	addiu	sp,sp,-40
 21c:	afbc0010 	sw	gp,16(sp)
 220:	afbf0020 	sw	ra,32(sp)
 224:	afbe001c 	sw	s8,28(sp)
 228:	afbc0018 	sw	gp,24(sp)
 22c:	03a0f021 	move	s8,sp
 230:	afc40028 	sw	a0,40(s8)
 234:	8fc40028 	lw	a0,40(s8)
 238:	8f990000 	lw	t9,0(gp)
 23c:	00000000 	nop
 240:	0320f809 	jalr	t9
 244:	00000000 	nop
 248:	8fdc0010 	lw	gp,16(s8)
 24c:	03c0e821 	move	sp,s8
 250:	8fbf0020 	lw	ra,32(sp)
 254:	8fbe001c 	lw	s8,28(sp)
 258:	03e00008 	jr	ra
 25c:	27bd0028 	addiu	sp,sp,40

00000260 <w_sesc_commit_>:
 260:	3c1c0000 	lui	gp,0x0
 264:	279c0000 	addiu	gp,gp,0
 268:	0399e021 	addu	gp,gp,t9
 26c:	27bdffd8 	addiu	sp,sp,-40
 270:	afbc0010 	sw	gp,16(sp)
 274:	afbf0020 	sw	ra,32(sp)
 278:	afbe001c 	sw	s8,28(sp)
 27c:	afbc0018 	sw	gp,24(sp)
 280:	03a0f021 	move	s8,sp
 284:	8f990000 	lw	t9,0(gp)
 288:	00000000 	nop
 28c:	0320f809 	jalr	t9
 290:	00000000 	nop
 294:	8fdc0010 	lw	gp,16(s8)
 298:	03c0e821 	move	sp,s8
 29c:	8fbf0020 	lw	ra,32(sp)
 2a0:	8fbe001c 	lw	s8,28(sp)
 2a4:	03e00008 	jr	ra
 2a8:	27bd0028 	addiu	sp,sp,40

000002ac <w_sesc_prof_commit_>:
 2ac:	3c1c0000 	lui	gp,0x0
 2b0:	279c0000 	addiu	gp,gp,0
 2b4:	0399e021 	addu	gp,gp,t9
 2b8:	27bdffd8 	addiu	sp,sp,-40
 2bc:	afbc0010 	sw	gp,16(sp)
 2c0:	afbf0020 	sw	ra,32(sp)
 2c4:	afbe001c 	sw	s8,28(sp)
 2c8:	afbc0018 	sw	gp,24(sp)
 2cc:	03a0f021 	move	s8,sp
 2d0:	afc40028 	sw	a0,40(s8)
 2d4:	8fc40028 	lw	a0,40(s8)
 2d8:	8f990000 	lw	t9,0(gp)
 2dc:	00000000 	nop
 2e0:	0320f809 	jalr	t9
 2e4:	00000000 	nop
 2e8:	8fdc0010 	lw	gp,16(s8)
 2ec:	03c0e821 	move	sp,s8
 2f0:	8fbf0020 	lw	ra,32(sp)
 2f4:	8fbe001c 	lw	s8,28(sp)
 2f8:	03e00008 	jr	ra
 2fc:	27bd0028 	addiu	sp,sp,40

00000300 <w_sesc_reg_com_>:
 300:	3c1c0000 	lui	gp,0x0
 304:	279c0000 	addiu	gp,gp,0
 308:	0399e021 	addu	gp,gp,t9
 30c:	27bdffd8 	addiu	sp,sp,-40
 310:	afbc0010 	sw	gp,16(sp)
 314:	afbf0020 	sw	ra,32(sp)
 318:	afbe001c 	sw	s8,28(sp)
 31c:	afbc0018 	sw	gp,24(sp)
 320:	03a0f021 	move	s8,sp
 324:	8f990000 	lw	t9,0(gp)
 328:	00000000 	nop
 32c:	0320f809 	jalr	t9
 330:	00000000 	nop
 334:	8fdc0010 	lw	gp,16(s8)
 338:	03c0e821 	move	sp,s8
 33c:	8fbf0020 	lw	ra,32(sp)
 340:	8fbe001c 	lw	s8,28(sp)
 344:	03e00008 	jr	ra
 348:	27bd0028 	addiu	sp,sp,40

0000034c <w_sesc_begin_versioning__>:
 34c:	3c1c0000 	lui	gp,0x0
 350:	279c0000 	addiu	gp,gp,0
 354:	0399e021 	addu	gp,gp,t9
 358:	27bdffd8 	addiu	sp,sp,-40
 35c:	afbc0010 	sw	gp,16(sp)
 360:	afbf0020 	sw	ra,32(sp)
 364:	afbe001c 	sw	s8,28(sp)
 368:	afbc0018 	sw	gp,24(sp)
 36c:	03a0f021 	move	s8,sp
 370:	8f990000 	lw	t9,0(gp)
 374:	00000000 	nop
 378:	0320f809 	jalr	t9
 37c:	00000000 	nop
 380:	8fdc0010 	lw	gp,16(s8)
 384:	03c0e821 	move	sp,s8
 388:	8fbf0020 	lw	ra,32(sp)
 38c:	8fbe001c 	lw	s8,28(sp)
 390:	03e00008 	jr	ra
 394:	27bd0028 	addiu	sp,sp,40

00000398 <w_sesc_fork_successor__>:
 398:	3c1c0000 	lui	gp,0x0
 39c:	279c0000 	addiu	gp,gp,0
 3a0:	0399e021 	addu	gp,gp,t9
 3a4:	27bdffd8 	addiu	sp,sp,-40
 3a8:	afbc0010 	sw	gp,16(sp)
 3ac:	afbf0020 	sw	ra,32(sp)
 3b0:	afbe001c 	sw	s8,28(sp)
 3b4:	afbc0018 	sw	gp,24(sp)
 3b8:	03a0f021 	move	s8,sp
 3bc:	8f990000 	lw	t9,0(gp)
 3c0:	00000000 	nop
 3c4:	0320f809 	jalr	t9
 3c8:	00000000 	nop
 3cc:	8fdc0010 	lw	gp,16(s8)
 3d0:	03c0e821 	move	sp,s8
 3d4:	8fbf0020 	lw	ra,32(sp)
 3d8:	8fbe001c 	lw	s8,28(sp)
 3dc:	03e00008 	jr	ra
 3e0:	27bd0028 	addiu	sp,sp,40

000003e4 <w_sesc_become_safe__>:
 3e4:	3c1c0000 	lui	gp,0x0
 3e8:	279c0000 	addiu	gp,gp,0
 3ec:	0399e021 	addu	gp,gp,t9
 3f0:	27bdffd8 	addiu	sp,sp,-40
 3f4:	afbc0010 	sw	gp,16(sp)
 3f8:	afbf0020 	sw	ra,32(sp)
 3fc:	afbe001c 	sw	s8,28(sp)
 400:	afbc0018 	sw	gp,24(sp)
 404:	03a0f021 	move	s8,sp
 408:	8f990000 	lw	t9,0(gp)
 40c:	00000000 	nop
 410:	0320f809 	jalr	t9
 414:	00000000 	nop
 418:	8fdc0010 	lw	gp,16(s8)
 41c:	03c0e821 	move	sp,s8
 420:	8fbf0020 	lw	ra,32(sp)
 424:	8fbe001c 	lw	s8,28(sp)
 428:	03e00008 	jr	ra
 42c:	27bd0028 	addiu	sp,sp,40

00000430 <w_sesc_end_versioning__>:
 430:	3c1c0000 	lui	gp,0x0
 434:	279c0000 	addiu	gp,gp,0
 438:	0399e021 	addu	gp,gp,t9
 43c:	27bdffd8 	addiu	sp,sp,-40
 440:	afbc0010 	sw	gp,16(sp)
 444:	afbf0020 	sw	ra,32(sp)
 448:	afbe001c 	sw	s8,28(sp)
 44c:	afbc0018 	sw	gp,24(sp)
 450:	03a0f021 	move	s8,sp
 454:	8f990000 	lw	t9,0(gp)
 458:	00000000 	nop
 45c:	0320f809 	jalr	t9
 460:	00000000 	nop
 464:	8fdc0010 	lw	gp,16(s8)
 468:	03c0e821 	move	sp,s8
 46c:	8fbf0020 	lw	ra,32(sp)
 470:	8fbe001c 	lw	s8,28(sp)
 474:	03e00008 	jr	ra
 478:	27bd0028 	addiu	sp,sp,40

0000047c <w_sesc_is_safe__>:
 47c:	3c1c0000 	lui	gp,0x0
 480:	279c0000 	addiu	gp,gp,0
 484:	0399e021 	addu	gp,gp,t9
 488:	27bdffd8 	addiu	sp,sp,-40
 48c:	afbc0010 	sw	gp,16(sp)
 490:	afbf0020 	sw	ra,32(sp)
 494:	afbe001c 	sw	s8,28(sp)
 498:	afbc0018 	sw	gp,24(sp)
 49c:	03a0f021 	move	s8,sp
 4a0:	afc40028 	sw	a0,40(s8)
 4a4:	8fc40028 	lw	a0,40(s8)
 4a8:	8f990000 	lw	t9,0(gp)
 4ac:	00000000 	nop
 4b0:	0320f809 	jalr	t9
 4b4:	00000000 	nop
 4b8:	8fdc0010 	lw	gp,16(s8)
 4bc:	03c0e821 	move	sp,s8
 4c0:	8fbf0020 	lw	ra,32(sp)
 4c4:	8fbe001c 	lw	s8,28(sp)
 4c8:	03e00008 	jr	ra
 4cc:	27bd0028 	addiu	sp,sp,40

000004d0 <w_sesc_commit__>:
 4d0:	3c1c0000 	lui	gp,0x0
 4d4:	279c0000 	addiu	gp,gp,0
 4d8:	0399e021 	addu	gp,gp,t9
 4dc:	27bdffd8 	addiu	sp,sp,-40
 4e0:	afbc0010 	sw	gp,16(sp)
 4e4:	afbf0020 	sw	ra,32(sp)
 4e8:	afbe001c 	sw	s8,28(sp)
 4ec:	afbc0018 	sw	gp,24(sp)
 4f0:	03a0f021 	move	s8,sp
 4f4:	8f990000 	lw	t9,0(gp)
 4f8:	00000000 	nop
 4fc:	0320f809 	jalr	t9
 500:	00000000 	nop
 504:	8fdc0010 	lw	gp,16(s8)
 508:	03c0e821 	move	sp,s8
 50c:	8fbf0020 	lw	ra,32(sp)
 510:	8fbe001c 	lw	s8,28(sp)
 514:	03e00008 	jr	ra
 518:	27bd0028 	addiu	sp,sp,40

0000051c <w_sesc_prof_commit__>:
 51c:	3c1c0000 	lui	gp,0x0
 520:	279c0000 	addiu	gp,gp,0
 524:	0399e021 	addu	gp,gp,t9
 528:	27bdffd8 	addiu	sp,sp,-40
 52c:	afbc0010 	sw	gp,16(sp)
 530:	afbf0020 	sw	ra,32(sp)
 534:	afbe001c 	sw	s8,28(sp)
 538:	afbc0018 	sw	gp,24(sp)
 53c:	03a0f021 	move	s8,sp
 540:	afc40028 	sw	a0,40(s8)
 544:	8fc40028 	lw	a0,40(s8)
 548:	8f990000 	lw	t9,0(gp)
 54c:	00000000 	nop
 550:	0320f809 	jalr	t9
 554:	00000000 	nop
 558:	8fdc0010 	lw	gp,16(s8)
 55c:	03c0e821 	move	sp,s8
 560:	8fbf0020 	lw	ra,32(sp)
 564:	8fbe001c 	lw	s8,28(sp)
 568:	03e00008 	jr	ra
 56c:	27bd0028 	addiu	sp,sp,40

00000570 <w_sesc_reg_com__>:
 570:	3c1c0000 	lui	gp,0x0
 574:	279c0000 	addiu	gp,gp,0
 578:	0399e021 	addu	gp,gp,t9
 57c:	27bdffd8 	addiu	sp,sp,-40
 580:	afbc0010 	sw	gp,16(sp)
 584:	afbf0020 	sw	ra,32(sp)
 588:	afbe001c 	sw	s8,28(sp)
 58c:	afbc0018 	sw	gp,24(sp)
 590:	03a0f021 	move	s8,sp
 594:	8f990000 	lw	t9,0(gp)
 598:	00000000 	nop
 59c:	0320f809 	jalr	t9
 5a0:	00000000 	nop
 5a4:	8fdc0010 	lw	gp,16(s8)
 5a8:	03c0e821 	move	sp,s8
 5ac:	8fbf0020 	lw	ra,32(sp)
 5b0:	8fbe001c 	lw	s8,28(sp)
 5b4:	03e00008 	jr	ra
 5b8:	27bd0028 	addiu	sp,sp,40

000005bc <w_sesc_begin_versioning>:
 5bc:	3c1c0000 	lui	gp,0x0
 5c0:	279c0000 	addiu	gp,gp,0
 5c4:	0399e021 	addu	gp,gp,t9
 5c8:	27bdffd8 	addiu	sp,sp,-40
 5cc:	afbc0010 	sw	gp,16(sp)
 5d0:	afbf0020 	sw	ra,32(sp)
 5d4:	afbe001c 	sw	s8,28(sp)
 5d8:	afbc0018 	sw	gp,24(sp)
 5dc:	03a0f021 	move	s8,sp
 5e0:	8f990000 	lw	t9,0(gp)
 5e4:	00000000 	nop
 5e8:	0320f809 	jalr	t9
 5ec:	00000000 	nop
 5f0:	8fdc0010 	lw	gp,16(s8)
 5f4:	03c0e821 	move	sp,s8
 5f8:	8fbf0020 	lw	ra,32(sp)
 5fc:	8fbe001c 	lw	s8,28(sp)
 600:	03e00008 	jr	ra
 604:	27bd0028 	addiu	sp,sp,40

00000608 <w_sesc_fork_successor>:
 608:	3c1c0000 	lui	gp,0x0
 60c:	279c0000 	addiu	gp,gp,0
 610:	0399e021 	addu	gp,gp,t9
 614:	27bdffd8 	addiu	sp,sp,-40
 618:	afbc0010 	sw	gp,16(sp)
 61c:	afbf0020 	sw	ra,32(sp)
 620:	afbe001c 	sw	s8,28(sp)
 624:	afbc0018 	sw	gp,24(sp)
 628:	03a0f021 	move	s8,sp
 62c:	8f990000 	lw	t9,0(gp)
 630:	00000000 	nop
 634:	0320f809 	jalr	t9
 638:	00000000 	nop
 63c:	8fdc0010 	lw	gp,16(s8)
 640:	03c0e821 	move	sp,s8
 644:	8fbf0020 	lw	ra,32(sp)
 648:	8fbe001c 	lw	s8,28(sp)
 64c:	03e00008 	jr	ra
 650:	27bd0028 	addiu	sp,sp,40

00000654 <w_sesc_become_safe>:
 654:	3c1c0000 	lui	gp,0x0
 658:	279c0000 	addiu	gp,gp,0
 65c:	0399e021 	addu	gp,gp,t9
 660:	27bdffd8 	addiu	sp,sp,-40
 664:	afbc0010 	sw	gp,16(sp)
 668:	afbf0020 	sw	ra,32(sp)
 66c:	afbe001c 	sw	s8,28(sp)
 670:	afbc0018 	sw	gp,24(sp)
 674:	03a0f021 	move	s8,sp
 678:	8f990000 	lw	t9,0(gp)
 67c:	00000000 	nop
 680:	0320f809 	jalr	t9
 684:	00000000 	nop
 688:	8fdc0010 	lw	gp,16(s8)
 68c:	03c0e821 	move	sp,s8
 690:	8fbf0020 	lw	ra,32(sp)
 694:	8fbe001c 	lw	s8,28(sp)
 698:	03e00008 	jr	ra
 69c:	27bd0028 	addiu	sp,sp,40

000006a0 <w_sesc_end_versioning>:
 6a0:	3c1c0000 	lui	gp,0x0
 6a4:	279c0000 	addiu	gp,gp,0
 6a8:	0399e021 	addu	gp,gp,t9
 6ac:	27bdffd8 	addiu	sp,sp,-40
 6b0:	afbc0010 	sw	gp,16(sp)
 6b4:	afbf0020 	sw	ra,32(sp)
 6b8:	afbe001c 	sw	s8,28(sp)
 6bc:	afbc0018 	sw	gp,24(sp)
 6c0:	03a0f021 	move	s8,sp
 6c4:	8f990000 	lw	t9,0(gp)
 6c8:	00000000 	nop
 6cc:	0320f809 	jalr	t9
 6d0:	00000000 	nop
 6d4:	8fdc0010 	lw	gp,16(s8)
 6d8:	03c0e821 	move	sp,s8
 6dc:	8fbf0020 	lw	ra,32(sp)
 6e0:	8fbe001c 	lw	s8,28(sp)
 6e4:	03e00008 	jr	ra
 6e8:	27bd0028 	addiu	sp,sp,40

000006ec <w_sesc_is_safe>:
 6ec:	3c1c0000 	lui	gp,0x0
 6f0:	279c0000 	addiu	gp,gp,0
 6f4:	0399e021 	addu	gp,gp,t9
 6f8:	27bdffd8 	addiu	sp,sp,-40
 6fc:	afbc0010 	sw	gp,16(sp)
 700:	afbf0020 	sw	ra,32(sp)
 704:	afbe001c 	sw	s8,28(sp)
 708:	afbc0018 	sw	gp,24(sp)
 70c:	03a0f021 	move	s8,sp
 710:	afc40028 	sw	a0,40(s8)
 714:	8fc40028 	lw	a0,40(s8)
 718:	8f990000 	lw	t9,0(gp)
 71c:	00000000 	nop
 720:	0320f809 	jalr	t9
 724:	00000000 	nop
 728:	8fdc0010 	lw	gp,16(s8)
 72c:	03c0e821 	move	sp,s8
 730:	8fbf0020 	lw	ra,32(sp)
 734:	8fbe001c 	lw	s8,28(sp)
 738:	03e00008 	jr	ra
 73c:	27bd0028 	addiu	sp,sp,40

00000740 <w_sesc_commit>:
 740:	3c1c0000 	lui	gp,0x0
 744:	279c0000 	addiu	gp,gp,0
 748:	0399e021 	addu	gp,gp,t9
 74c:	27bdffd8 	addiu	sp,sp,-40
 750:	afbc0010 	sw	gp,16(sp)
 754:	afbf0020 	sw	ra,32(sp)
 758:	afbe001c 	sw	s8,28(sp)
 75c:	afbc0018 	sw	gp,24(sp)
 760:	03a0f021 	move	s8,sp
 764:	8f990000 	lw	t9,0(gp)
 768:	00000000 	nop
 76c:	0320f809 	jalr	t9
 770:	00000000 	nop
 774:	8fdc0010 	lw	gp,16(s8)
 778:	03c0e821 	move	sp,s8
 77c:	8fbf0020 	lw	ra,32(sp)
 780:	8fbe001c 	lw	s8,28(sp)
 784:	03e00008 	jr	ra
 788:	27bd0028 	addiu	sp,sp,40

0000078c <w_sesc_prof_commit>:
 78c:	3c1c0000 	lui	gp,0x0
 790:	279c0000 	addiu	gp,gp,0
 794:	0399e021 	addu	gp,gp,t9
 798:	27bdffd8 	addiu	sp,sp,-40
 79c:	afbc0010 	sw	gp,16(sp)
 7a0:	afbf0020 	sw	ra,32(sp)
 7a4:	afbe001c 	sw	s8,28(sp)
 7a8:	afbc0018 	sw	gp,24(sp)
 7ac:	03a0f021 	move	s8,sp
 7b0:	afc40028 	sw	a0,40(s8)
 7b4:	8fc40028 	lw	a0,40(s8)
 7b8:	8f990000 	lw	t9,0(gp)
 7bc:	00000000 	nop
 7c0:	0320f809 	jalr	t9
 7c4:	00000000 	nop
 7c8:	8fdc0010 	lw	gp,16(s8)
 7cc:	03c0e821 	move	sp,s8
 7d0:	8fbf0020 	lw	ra,32(sp)
 7d4:	8fbe001c 	lw	s8,28(sp)
 7d8:	03e00008 	jr	ra
 7dc:	27bd0028 	addiu	sp,sp,40

000007e0 <w_sesc_reg_com>:
 7e0:	3c1c0000 	lui	gp,0x0
 7e4:	279c0000 	addiu	gp,gp,0
 7e8:	0399e021 	addu	gp,gp,t9
 7ec:	27bdffd8 	addiu	sp,sp,-40
 7f0:	afbc0010 	sw	gp,16(sp)
 7f4:	afbf0020 	sw	ra,32(sp)
 7f8:	afbe001c 	sw	s8,28(sp)
 7fc:	afbc0018 	sw	gp,24(sp)
 800:	03a0f021 	move	s8,sp
 804:	8f990000 	lw	t9,0(gp)
 808:	00000000 	nop
 80c:	0320f809 	jalr	t9
 810:	00000000 	nop
 814:	8fdc0010 	lw	gp,16(s8)
 818:	03c0e821 	move	sp,s8
 81c:	8fbf0020 	lw	ra,32(sp)
 820:	8fbe001c 	lw	s8,28(sp)
 824:	03e00008 	jr	ra
 828:	27bd0028 	addiu	sp,sp,40
 82c:	00000000 	nop
Disassembly of section .data:
Disassembly of section .reginfo:

00000000 <.reginfo>:
   0:	f2ff0010 	0xf2ff0010
   4:	00000000 	nop
   8:	fff00000 	0xfff00000
	...
