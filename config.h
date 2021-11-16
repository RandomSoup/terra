#ifndef CONFIG_H
#define CONFIG_H

#define FONTW 8
#define FONTH 16
#define XOFF 8
#define YOFF 8
#define MAXC 96
#define MAXR 38
#define RETRIES 3
#define BGCOLOR 0x25252525
#define TIMEOUT 30

#ifdef DEF_PALETTE
static uint32_t palette[] = {
	0xffffff, 0xffffff, 0xffffff,
	0xbb8888, 0xbbbbbb, 0x22dddd,
	0xbbbbbb, 0x88bb88, 0x888888
};
#undef DEF_PALETTE
#endif

#ifdef DEF_ABOUT
/* #embed when? */
static const char* about =
	"# Welcome to Rover " VERSION "!\n" \
	"You can enter any Piper URL in the address bar, and press Enter to navigate, " \
	"and the up and down arrows to scroll.\n\n" \
	"## Page index\n\n" \
	"=> piper://citadel.luminoso.dev Luminoso.dev - Piper Edition\n\n" \
	"## License & copyright\n\n" \
	"> Rover is licensed under an ISC-like license.\n" \
	"> The Spleen font by Frederic Cambus is licensed under the Simplified BSD License.\n"
;
#undef DEF_ABOUT
#endif

#endif /* !CONFIG_H */
