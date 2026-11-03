#ifndef __COMPAT_6_18_H__
#define __COMPAT_6_18_H__

#include <linux/version.h>
#include <linux/mm.h>

#ifndef nth_page
#define nth_page(page, n) pfn_to_page(page_to_pfn((page)) + (n))
#endif

#endif /* __COMPAT_6_18_H__ */
