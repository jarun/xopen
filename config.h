/* See LICENSE file for copyright and license details. */

#define IMAGE_INDEX 0
#define AUDIO_INDEX 1
#define VIDEO_INDEX 2
#define PDF_INDEX   3

#define AUDIO_PLAYER "mpv"
#define FALLBACK_CMD "xdg-open"

#define P(RE,...) { RE, (const char*[]) { __VA_ARGS__, NULL} }

//#define WEB_PREFIX(URL) "^(https?://www\\." URL "|https?://" URL ")"

static const Pair pairs[] = {
	/* regex                  action */

	/* image files */
	P( "\\.(jpe?g|png|gif|webp|tiff|bmp|ico|svg)$", "gpicview", "%s" ),

	/* audio: copy the nnn plugin mocq to /usr/local/bin/ */
	P( "\\.(aac|flac|m4a|mid|midi|mpa|mp2|mp3|ogg|wav|wma)$",
		"sh", "-c", "if command -v mocq >/dev/null 2>&1; then exec mocq \"$1\" opener; elif command -v mocp >/dev/null 2>&1; then exec mocp -a \"$1\"; else exec AUDIO_PLAYER \"$1\"; fi", "audio-open", "%s" ),

	/* video */
	P( "\\.(avi|mkv|mp4|mov|webm|m4v)$", AUDIO_PLAYER, "%s" ),

	/* pdf */
	P( "\\.(pdf)$", "zathura", "%s" ),

	/* Office document (WPS Office) */
	P( "\\.(doc|docx|rtf)$", "wps", "%s" ),
	P( "\\.(xls|xlsx|csv)$", "et", "%s" ),
	P( "\\.(ppt|pptx)$", "wpp", "%s" ),

	/* text (ignored - use nnn options -e and -E to let nnn open text files in the terminal) */
	//P( "\\.(txt|md|rst)$", "mousepad", "%s" ),
	//P( "\\.(htm|html|xhtml)$", FALLBACK_CMD, "%s" ),

	/* web */
	// P( "^(mailto:|https?://|ftp://)", "xdg-open", "%s" ),

	/* youtube */
	// P( WEB_PREFIX("youtube.com/watch\\?|youtu\\.be/"), AUDIO_PLAYER, "%s"),
	// P( WEB_PREFIX("github.com"), "xdg-open", "%s" ),
};

#undef P
//#undef WEB_PREFIX
