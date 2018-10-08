#ifndef _TCCECONFIG_H
#define _TCCECONFIG_H

struct TermProps
{
};

struct TCCECFG
{
	char * thisfile;
	char * dotcfile;

	

	int nrterms;
	struct TermProps * tps;
}
#endif

