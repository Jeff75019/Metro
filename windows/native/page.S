/* La page de l'application (dist/KeoLive.html, un seul fichier), incluse dans l'exécutable
   comme une simple donnée en lecture seule : keolive_page .. keolive_page_end. */
	.section .rdata,"dr"
	.p2align 4
	.globl keolive_page
keolive_page:
	.incbin "dist/KeoLive.html"
	.globl keolive_page_end
keolive_page_end:
	.byte 0
