/* config.h.  Generated from config.h.in by configure.  */
/* config.h.in.  Generated from configure.ac by autoheader.  */

/* Define if building universal (internal helper macro) */
/* #undef AC_APPLE_UNIVERSAL_BUILD */

/* Have ARM atomic instructions. */
/* #undef ATOMIC_ARM_INLINE_ASM */

/* special arm linux implementation */
/* #undef ATOMIC_ARM_LINUX_HELPERS */

/* Enable memory barriers */
/* #undef ATOMIC_ARM_MEMORY_BARRIER_ENABLED */

/* Canonical host string. */
#define CANONICAL_HOST "x86_64-pc-linux-gnu"

/* Define to one of `_getb67', `GETB67', `getb67' for Cray-2 and Cray-YMP
   systems. This function is required for `alloca.c' support on those systems.
   */
/* #undef CRAY_STACKSEG_END */

/* Define to 1 if using `alloca.c'. */
/* #undef C_ALLOCA */

/* Disable Orc */
#define DISABLE_ORC 1

/* Legacy database entry format */
#define ENABLE_LEGACY_DATABASE_ENTRY_FORMAT 1

/* Define to 1 if translation of program messages to the user's native
   language is requested. */
#define ENABLE_NLS 1

/* Define to the type of elements in the array set by `getgroups'. Usually
   this is either `int' or `gid_t'. */
#define GETGROUPS_T gid_t

/* Gettext package */
#define GETTEXT_PACKAGE "pulseaudio"

/* Define to 1 if you have the `accept4' function. */
#define HAVE_ACCEPT4 1

/* Define to 1 if you have `alloca', as a function or macro. */
#define HAVE_ALLOCA 1

/* Define to 1 if you have <alloca.h> and it should be used (not on Ultrix).
   */
#define HAVE_ALLOCA_H 1

/* Have ALSA? */
#define HAVE_ALSA 1

/* Have ALSA UCM? */
#define HAVE_ALSA_UCM 1

/* Define to 1 if you have the <alsa/use-case.h> header file. */
#define HAVE_ALSA_USE_CASE_H 1

/* Have ARMv6 instructions. */
/* #undef HAVE_ARMV6 */

/* Define to 1 if you have the <arpa/inet.h> header file. */
#define HAVE_ARPA_INET_H 1

/* Have __sync_bool_compare_and_swap() and friends. */
#define HAVE_ATOMIC_BUILTINS 1

/* Have __atomic_store_n() and friends. */
#define HAVE_ATOMIC_BUILTINS_MEMORY_MODEL 1

/* Define to 1 if you have the <atomic_ops.h> header file. */
/* #undef HAVE_ATOMIC_OPS_H */

/* Define to 1 if immediate binding is available */
#define HAVE_BIND_NOW 1

/* Bluez 5 native headset backend enabled */
/* #undef HAVE_BLUEZ_5_NATIVE_HEADSET */

/* Bluez 5 ofono headset backend enabled */
/* #undef HAVE_BLUEZ_5_OFONO_HEADSET */

/* Define to 1 if you have the <byteswap.h> header file. */
#define HAVE_BYTESWAP_H 1

/* Define to 1 if you have the Mac OS X function CFLocaleCopyCurrent in the
   CoreFoundation framework. */
/* #undef HAVE_CFLOCALECOPYCURRENT */

/* Define to 1 if you have the Mac OS X function CFPreferencesCopyAppValue in
   the CoreFoundation framework. */
/* #undef HAVE_CFPREFERENCESCOPYAPPVALUE */

/* Define to 1 if you have the `chmod' function. */
#define HAVE_CHMOD 1

/* Define to 1 if you have the `chown' function. */
#define HAVE_CHOWN 1

/* Define to 1 if you have the `clock_gettime' function. */
#define HAVE_CLOCK_GETTIME 1

/* Have CoreAudio? */
/* #undef HAVE_COREAUDIO */

/* Define to 1 if you have the <CoreAudio/CoreAudio.h> header file. */
/* #undef HAVE_COREAUDIO_COREAUDIO_H */

/* Define to 1 if you have the <cpuid.h> header file. */
#define HAVE_CPUID_H 1

/* Define to 1 if you have the `ctime_r' function. */
#define HAVE_CTIME_R 1

/* define if the compiler supports basic C++11 syntax */
#define HAVE_CXX11 1

/* Have D-Bus. */
/* #undef HAVE_DBUS */

/* Define if the GNU dcgettext() function is already present or preinstalled.
   */
#define HAVE_DCGETTEXT 1

/* Define to 1 if you have the declaration of `environ', and to 0 if you
   don't. */
#define HAVE_DECL_ENVIRON 1

/* Define to 1 if you have the declaration of `SOUND_PCM_READ_BITS', and to 0
   if you don't. */
#define HAVE_DECL_SOUND_PCM_READ_BITS 1

/* Define to 1 if you have the declaration of `SOUND_PCM_READ_CHANNELS', and
   to 0 if you don't. */
#define HAVE_DECL_SOUND_PCM_READ_CHANNELS 1

/* Define to 1 if you have the declaration of `SOUND_PCM_READ_RATE', and to 0
   if you don't. */
#define HAVE_DECL_SOUND_PCM_READ_RATE 1

/* Have dladdr? */
#define HAVE_DLADDR 1

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1

/* Define to 1 if you have the <execinfo.h> header file. */
#define HAVE_EXECINFO_H 1

/* Have CPU with fast 64-bit operations? */
#define HAVE_FAST_64BIT_OPERATIONS 1

/* Define to 1 if you have the `fchmod' function. */
#define HAVE_FCHMOD 1

/* Define to 1 if you have the `fchown' function. */
#define HAVE_FCHOWN 1

/* Define to 1 if you have the `fork' function. */
#define HAVE_FORK 1

/* Define to 1 if you have the `fstat' function. */
#define HAVE_FSTAT 1

/* Have gdbm? */
/* #undef HAVE_GDBM */

/* Define to 1 if you have the <gdbm.h> header file. */
/* #undef HAVE_GDBM_H */

/* Define to 1 if you have the `getaddrinfo' function. */
#define HAVE_GETADDRINFO 1

/* Define to 1 if you have the `getgrgid_r' function. */
#define HAVE_GETGRGID_R 1

/* Define to 1 if you have the `getgrnam_r' function. */
#define HAVE_GETGRNAM_R 1

/* Define to 1 if your system has a working `getgroups' function. */
#define HAVE_GETGROUPS 1

/* Define to 1 if you have the `getopt_long' function. */
#define HAVE_GETOPT_LONG 1

/* Define to 1 if you have the `getpwnam_r' function. */
#define HAVE_GETPWNAM_R 1

/* Define to 1 if you have the `getpwuid_r' function. */
#define HAVE_GETPWUID_R 1

/* Define if the GNU gettext() function is already present or preinstalled. */
#define HAVE_GETTEXT 1

/* Define to 1 if you have the `gettimeofday' function. */
#define HAVE_GETTIMEOFDAY 1

/* Define to 1 if you have the `getuid' function. */
#define HAVE_GETUID 1

/* Have GLIB? */
/* #undef HAVE_GLIB */

/* Define to 1 if you have the <glob.h> header file. */
#define HAVE_GLOB_H 1

/* Define to 1 if you have the <grp.h> header file. */
#define HAVE_GRP_H 1

/* Have GTK? */
/* #undef HAVE_GTK */

/* Have HAL compatibility. */
#define HAVE_HAL_COMPAT 1

/* Define if you have the iconv() function and it works. */
#define HAVE_ICONV 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define this to enable IPv6 connection support */
#define HAVE_IPV6 1

/* Define to 1 if you have the <langinfo.h> header file. */
#define HAVE_LANGINFO_H 1

/* Have libasyncns? */
/* #undef HAVE_LIBASYNCNS */

/* Define to 1 if you have the `gdbm' library (-lgdbm). */
/* #undef HAVE_LIBGDBM */

/* Define to 1 if you have the `iberty' library (-liberty). */
/* #undef HAVE_LIBIBERTY */

/* Have libsamplerate? */
/* #undef HAVE_LIBSAMPLERATE */

/* Have tcpwrap? */
/* #undef HAVE_LIBWRAP */

/* Define to 1 if you have the <linux/input.h> header file. */
#define HAVE_LINUX_INPUT_H 1

/* Define to 1 if you have the <linux/sockios.h> header file. */
#define HAVE_LINUX_SOCKIOS_H 1

/* Define to 1 if you have the <locale.h> header file. */
#define HAVE_LOCALE_H 1

/* Define to 1 if you have the `lrintf' function. */
#define HAVE_LRINTF 1

/* Define to 1 if you have the `lstat' function. */
#define HAVE_LSTAT 1

/* Have memfd shared memory. */
#define HAVE_MEMFD 1

/* Define to 1 if you have the `memfd_create' function. */
/* #undef HAVE_MEMFD_CREATE */

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the `mkfifo' function. */
#define HAVE_MKFIFO 1

/* Define to 1 if you have the `mlock' function. */
#define HAVE_MLOCK 1

/* Define to 1 if you have the <mmsystem.h> header file. */
/* #undef HAVE_MMSYSTEM_H */

/* Define to 1 if you have the `nanosleep' function. */
#define HAVE_NANOSLEEP 1

/* Have NEON support? */
/* #undef HAVE_NEON */

/* Define to 1 if you have the <netdb.h> header file. */
#define HAVE_NETDB_H 1

/* Define to 1 if you have the <netinet/in.h> header file. */
#define HAVE_NETINET_IN_H 1

/* Define to 1 if you have the <netinet/in_systm.h> header file. */
#define HAVE_NETINET_IN_SYSTM_H 1

/* Define to 1 if you have the <netinet/ip.h> header file. */
#define HAVE_NETINET_IP_H 1

/* Define to 1 if you have the <netinet/tcp.h> header file. */
#define HAVE_NETINET_TCP_H 1

/* Define to 1 if you have the `open64' function. */
#define HAVE_OPEN64 1

/* Have OpenSSL */
#define HAVE_OPENSSL 1

/* Use Orc */
/* #undef HAVE_ORC */

/* Have OSS output? */
#define HAVE_OSS_OUTPUT 1

/* Have OSS wrapper (padsp)? */
#define HAVE_OSS_WRAPPER 1

/* Define to 1 if you have the `paccept' function. */
/* #undef HAVE_PACCEPT */

/* Define to 1 if you have the <pcreposix.h> header file. */
/* #undef HAVE_PCREPOSIX_H */

/* Define to 1 if you have the `pipe' function. */
#define HAVE_PIPE 1

/* Define to 1 if you have the `pipe2' function. */
#define HAVE_PIPE2 1

/* Define to 1 if you have the <poll.h> header file. */
#define HAVE_POLL_H 1

/* Define to 1 if you have the `posix_fadvise' function. */
#define HAVE_POSIX_FADVISE 1

/* Define to 1 if you have the `posix_madvise' function. */
#define HAVE_POSIX_MADVISE 1

/* Define to 1 if you have the `posix_memalign' function. */
#define HAVE_POSIX_MEMALIGN 1

/* Define to 1 if you have the `ppoll' function. */
#define HAVE_PPOLL 1

/* Define if you have POSIX threads libraries and header files. */
#define HAVE_PTHREAD 1

/* Define to 1 if you have the `pthread_getname_np' function. */
#define HAVE_PTHREAD_GETNAME_NP 1

/* Have PTHREAD_PRIO_INHERIT. */
#define HAVE_PTHREAD_PRIO_INHERIT 1

/* Define to 1 if you have the `pthread_setaffinity_np' function. */
#define HAVE_PTHREAD_SETAFFINITY_NP 1

/* Define to 1 if you have the `pthread_setname_np' function. */
#define HAVE_PTHREAD_SETNAME_NP 1

/* Define to 1 if you have the <pwd.h> header file. */
#define HAVE_PWD_H 1

/* Define to 1 if you have the `readlink' function. */
#define HAVE_READLINK 1

/* Define to 1 if you have the `regexec' function. */
#define HAVE_REGEXEC 1

/* Define to 1 if you have the <regex.h> header file. */
#define HAVE_REGEX_H 1

/* Have running from build tree */
#define HAVE_RUNNING_FROM_BUILD_TREE 1

/* Define to 1 if you have the <sched.h> header file. */
#define HAVE_SCHED_H 1

/* Define to 1 if you have the `setegid' function. */
#define HAVE_SETEGID 1

/* Define to 1 if you have the `seteuid' function. */
#define HAVE_SETEUID 1

/* Define to 1 if you have the `setpgid' function. */
#define HAVE_SETPGID 1

/* Define to 1 if you have the `setregid' function. */
#define HAVE_SETREGID 1

/* Define to 1 if you have the `setresgid' function. */
#define HAVE_SETRESGID 1

/* Define to 1 if you have the `setresuid' function. */
#define HAVE_SETRESUID 1

/* Define to 1 if you have the `setreuid' function. */
#define HAVE_SETREUID 1

/* Define to 1 if you have the `setsid' function. */
#define HAVE_SETSID 1

/* Define to 1 if you have the `shm_open' function. */
#define HAVE_SHM_OPEN 1

/* Define to 1 if you have the `sig2str' function. */
/* #undef HAVE_SIG2STR */

/* Define to 1 if you have the `sigaction' function. */
#define HAVE_SIGACTION 1

/* Have SIGXCPU? */
#define HAVE_SIGXCPU 1

/* Have simple? */
#define HAVE_SIMPLEDB 1

/* Define to 1 if you have the `sleep' function. */
#define HAVE_SLEEP 1

/* Have Solaris audio? */
/* #undef HAVE_SOLARIS */

/* Have soxr */
/* #undef HAVE_SOXR */

/* Have speex */
/* #undef HAVE_SPEEX */

/* Define to 1 if the system has the type `ssize_t'. */
#define HAVE_SSIZE_T 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Have _Bool. */
#define HAVE_STD_BOOL 1

/* Define to 1 if you have the `strerror_r' function. */
#define HAVE_STRERROR_R 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the `strsignal' function. */
#define HAVE_STRSIGNAL 1

/* Define to 1 if you have the `strtod_l' function. */
#define HAVE_STRTOD_L 1

/* Define to 1 if you have the `strtof' function. */
#define HAVE_STRTOF 1

/* Define to 1 if you have the `symlink' function. */
#define HAVE_SYMLINK 1

/* Define to 1 if you have the `sysconf' function. */
#define HAVE_SYSCONF 1

/* Define to 1 if you have the <syslog.h> header file. */
#define HAVE_SYSLOG_H 1

/* Have SYSTEMDDAEMON? */
/* #undef HAVE_SYSTEMD_DAEMON */

/* Have SYSTEMDJOURNAL? */
/* #undef HAVE_SYSTEMD_JOURNAL */

/* Have SYSTEMDLOGIN? */
/* #undef HAVE_SYSTEMD_LOGIN */

/* Define to 1 if you have the <sys/atomic.h> header file. */
/* #undef HAVE_SYS_ATOMIC_H */

/* Define to 1 if you have the <sys/audio.h> header file. */
/* #undef HAVE_SYS_AUDIO_H */

/* Define to 1 if you have the <sys/capability.h> header file. */
/* #undef HAVE_SYS_CAPABILITY_H */

/* Define to 1 if you have the <sys/conf.h> header file. */
/* #undef HAVE_SYS_CONF_H */

/* Define to 1 if you have the <sys/dl.h> header file. */
/* #undef HAVE_SYS_DL_H */

/* Define to 1 if you have the <sys/eventfd.h> header file. */
#define HAVE_SYS_EVENTFD_H 1

/* Define to 1 if you have the <sys/filio.h> header file. */
/* #undef HAVE_SYS_FILIO_H */

/* Define to 1 if you have the <sys/ioctl.h> header file. */
#define HAVE_SYS_IOCTL_H 1

/* Define to 1 if you have the <sys/mman.h> header file. */
#define HAVE_SYS_MMAN_H 1

/* Define to 1 if you have the <sys/prctl.h> header file. */
#define HAVE_SYS_PRCTL_H 1

/* Define to 1 if you have the <sys/resource.h> header file. */
#define HAVE_SYS_RESOURCE_H 1

/* Define to 1 if you have the <sys/select.h> header file. */
#define HAVE_SYS_SELECT_H 1

/* Define to 1 if you have the <sys/socket.h> header file. */
#define HAVE_SYS_SOCKET_H 1

/* Define to 1 if you have the <sys/soundcard.h> header file. */
#define HAVE_SYS_SOUNDCARD_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/syscall.h> header file. */
#define HAVE_SYS_SYSCALL_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <sys/uio.h> header file. */
#define HAVE_SYS_UIO_H 1

/* Define to 1 if you have the <sys/un.h> header file. */
#define HAVE_SYS_UN_H 1

/* Define to 1 if you have the <sys/wait.h> header file. */
#define HAVE_SYS_WAIT_H 1

/* Have tdb? */
/* #undef HAVE_TDB */

/* Have UDEV. */
#define HAVE_UDEV 1

/* Define to 1 if you have the `uname' function. */
#define HAVE_UNAME 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to 1 if you have the `usleep' function. */
#define HAVE_USLEEP 1

/* Define to 1 if you have the <valgrind/memcheck.h> header file. */
/* #undef HAVE_VALGRIND_MEMCHECK_H */

/* Define to 1 if you have the `vfork' function. */
#define HAVE_VFORK 1

/* Define to 1 if you have the <vfork.h> header file. */
/* #undef HAVE_VFORK_H */

/* Have WaveOut audio? */
/* #undef HAVE_WAVEOUT */

/* Define to 1 if you have the <windows.h> header file. */
/* #undef HAVE_WINDOWS_H */

/* Define to 1 if you have the <winsock2.h> header file. */
/* #undef HAVE_WINSOCK2_H */

/* Define to 1 if `fork' works. */
#define HAVE_WORKING_FORK 1

/* Define to 1 if `vfork' works. */
#define HAVE_WORKING_VFORK 1

/* Define to 1 if you have the <ws2tcpip.h> header file. */
/* #undef HAVE_WS2TCPIP_H */

/* Have X11? */
/* #undef HAVE_X11 */

/* Define to 1 if you have the <xlocale.h> header file. */
#define HAVE_XLOCALE_H 1

/* Define as const if the declaration of iconv() needs const. */
#define ICONV_CONST 

/* Define INADDR_NONE if not found in <netinet/in.h> */
/* #undef INADDR_NONE */

/* Define to the sub-directory where libtool stores uninstalled libraries. */
#define LT_OBJDIR ".libs/"

/* Build target is Darwin. */
/* #undef OS_IS_DARWIN */

/* Build target is Windows. */
/* #undef OS_IS_WIN32 */

/* Name of package */
#define PACKAGE "pulseaudio"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "pulseaudio-discuss (at) lists (dot) freedesktop (dot) org"

/* Define to the full name of this package. */
#define PACKAGE_NAME "pulseaudio"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "pulseaudio 13.0"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "pulseaudio"

/* Define to the home page for this package. */
#define PACKAGE_URL "http://pulseaudio.org/"

/* Define to the version of this package. */
#define PACKAGE_VERSION "13.0"

/* Location of pactl binary */
#define PACTL_BINARY "/usr/local/bin/pactl"

/* Access group */
#define PA_ACCESS_GROUP "pulse-access"

/* Location of pulseaudio binary */
#define PA_BINARY "/usr/local/bin/pulseaudio"

/* The CFLAGS used during compilation */
#define PA_CFLAGS "-g -O2 -Wall -W -Wextra -pipe -Wno-long-long -Wno-overlength-strings -Wunsafe-loop-optimizations -Wundef -Wformat=2 -Wlogical-op -Wsign-compare -Wformat-security -Wmissing-include-dirs -Wformat-nonliteral -Wpointer-arith -Winit-self -Wdeclaration-after-statement -Wfloat-equal -Wmissing-prototypes -Wredundant-decls -Wmissing-declarations -Wmissing-noreturn -Wshadow -Wendif-labels -Wcast-align -Wstrict-aliasing -Wwrite-strings -Wno-unused-parameter -fno-common -fdiagnostics-show-option -fdiagnostics-color=auto"

/* Location of configuration files */
#define PA_DEFAULT_CONFIG_DIR "/usr/local/etc/pulse"

/* Modules dir */
#define PA_DLSEARCHPATH "/usr/local/lib/pulse-13.0/modules"

/* PulseAudio include dir */
#define PA_INCDIR "/usr/local/include"

/* PulseAudio library dir */
#define PA_LIBDIR "/usr/local/lib"

/* D-Bus machine-id file */
#define PA_MACHINE_ID "/usr/local/etc/machine-id"

/* Fallback machine-id file */
#define PA_MACHINE_ID_FALLBACK "/usr/local/var/lib/dbus/machine-id"

/* Shared object extension */
#define PA_SOEXT ".so"

/* System config dir */
#define PA_SYSTEM_CONFIG_PATH "/usr/local/var/lib/pulse"

/* Group for the PulseAudio system daemon */
#define PA_SYSTEM_GROUP "pulse"

/* System runtime dir */
#define PA_SYSTEM_RUNTIME_PATH "/usr/local/var/run/pulse"

/* System state dir */
#define PA_SYSTEM_STATE_PATH "/usr/local/var/lib/pulse"

/* User for running the PulseAudio system daemon */
#define PA_SYSTEM_USER "pulse"

/* Define to necessary symbol if this constant uses a non-standard name on
   your system. */
/* #undef PTHREAD_CREATE_JOINABLE */

/* The size of `void*', as computed by sizeof. */
#define SIZEOF_VOIDP 8

/* If using the C implementation of alloca, define if you know the
   direction of stack growth for your system; otherwise it will be
   automatically deduced at runtime.
	STACK_DIRECTION > 0 => grows toward higher addresses
	STACK_DIRECTION < 0 => grows toward lower addresses
	STACK_DIRECTION = 0 => direction of growth unknown */
/* #undef STACK_DIRECTION */

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Define this if the compiler supports __thread for Thread-Local Storage */
#define SUPPORT_TLS___THREAD 1

/* If the compiler supports a TLS storage class define it to that here */
#define TLS __thread

/* Define this if you want per-user esound socket directories */
#define USE_PER_USER_ESOUND_SOCKET 1

/* Enable extensions on AIX 3, Interix.  */
#ifndef _ALL_SOURCE
# define _ALL_SOURCE 1
#endif
/* Enable GNU extensions on systems that have them.  */
#ifndef _GNU_SOURCE
# define _GNU_SOURCE 1
#endif
/* Enable threading extensions on Solaris.  */
#ifndef _POSIX_PTHREAD_SEMANTICS
# define _POSIX_PTHREAD_SEMANTICS 1
#endif
/* Enable extensions on HP NonStop.  */
#ifndef _TANDEM_SOURCE
# define _TANDEM_SOURCE 1
#endif
/* Enable general extensions on Solaris.  */
#ifndef __EXTENSIONS__
# define __EXTENSIONS__ 1
#endif


/* Version number of package */
#define VERSION "13.0"

/* Needed to avoid including unnecessary headers on Windows */
/* #undef WIN32_LEAN_AND_MEAN */

/* Define WORDS_BIGENDIAN to 1 if your processor stores words with the most
   significant byte first (like Motorola and SPARC, unlike Intel). */
#if defined AC_APPLE_UNIVERSAL_BUILD
# if defined __BIG_ENDIAN__
#  define WORDS_BIGENDIAN 1
# endif
#else
# ifndef WORDS_BIGENDIAN
/* #  undef WORDS_BIGENDIAN */
# endif
#endif

/* Needed to get NSIG on Mac OS X */
/* #undef _DARWIN_C_SOURCE */

/* Enable large inode numbers on Mac OS X 10.5.  */
#ifndef _DARWIN_USE_64_BIT_INODE
# define _DARWIN_USE_64_BIT_INODE 1
#endif

/* Number of bits in a file offset, on hosts where this is settable. */
/* #undef _FILE_OFFSET_BITS */

/* Define for large files, on AIX-style hosts. */
/* #undef _LARGE_FILES */

/* Define to 1 if on MINIX. */
/* #undef _MINIX */

/* Define to 2 if the system does not provide POSIX.1 features except with
   this defined. */
/* #undef _POSIX_1_SOURCE */

/* Needed on Solaris */
#define _POSIX_PTHREAD_SEMANTICS 1

/* Define to 1 if you need to in order for `stat' and other things to work. */
/* #undef _POSIX_SOURCE */

/* Needed to get declarations for msg_control and msg_controllen on Solaris */
/* #undef _XOPEN_SOURCE */

/* Needed to get declarations for msg_control and msg_controllen on Solaris */
#define __EXTENSIONS__ 1

/* Define to empty if `const' does not conform to ANSI C. */
/* #undef const */

/* Define to `int' if <sys/types.h> doesn't define. */
/* #undef gid_t */

/* Define to `long int' if <sys/types.h> does not define. */
/* #undef off_t */

/* Define to `int' if <sys/types.h> does not define. */
/* #undef pid_t */

/* Define to `unsigned int' if <sys/types.h> does not define. */
/* #undef size_t */

/* Define ssize_t if it is not done by the standard libs. */
/* #undef ssize_t */

/* Define to `int' if <sys/types.h> doesn't define. */
/* #undef uid_t */

/* Define as `fork' if `vfork' does not work. */
/* #undef vfork */
