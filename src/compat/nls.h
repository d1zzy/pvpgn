/* from mkdir */
/* TODO.... no work other than this has been done for internationalization */
#if HAVE_LOCALE_H  
# include <locale.h>  
#endif  
#if !HAVE_SETLOCALE  
# define setlocale(Category, Locale) /* empty */  
#endif  

#if ENABLE_NLS  
# include <libintl.h>  
# define _(Text) gettext (Text)  
#else  
# undef bindtextdomain  
# define bindtextdomain(Domain, Directory) /* empty */  
# undef textdomain  
# define textdomain(Domain) /* empty */  
# define _(Text) Text  
#endif  
