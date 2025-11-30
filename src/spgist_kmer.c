#include "postgres.h"
#include "fmgr.h"
#include "access/spgist.h"
#include "access/stratnum.h"
#include "utils/builtins.h"
#include "catalog/pg_type.h"
#include "utils/varlena.h"
#include "access/spgist.h"
#include "varatt.h"
#include "kmer.h"
#include "qkmer.h"

#define KMER_QKMER_CONTAINS_STRATEGY 10
#define KMER_PREFIX_CONTAINS_STRATEGY 28
/**
* implementation of the generic spgist functions
**/

PG_FUNCTION_INFO_V1(spg_kmer_config);
PG_FUNCTION_INFO_V1(spg_kmer_choose);
PG_FUNCTION_INFO_V1(spg_kmer_picksplit);
PG_FUNCTION_INFO_V1(spg_kmer_inner_consistent);
PG_FUNCTION_INFO_V1(spg_kmer_leaf_consistent);


Datum
spg_kmer_config(PG_FUNCTION_ARGS)
{
    spgConfigIn  *cfgin = (spgConfigIn *) PG_GETARG_POINTER(0);
    spgConfigOut *cfg   = (spgConfigOut *) PG_GETARG_POINTER(1);

    cfg->prefixType = VOIDOID;//empty
    cfg->labelType = VOIDOID; // detrminist so we don't need labels see helper function bellow

     // InvalidOid means “same as column type”.
    cfg->leafType = InvalidOid;

    cfg->canReturnData = true;
    cfg->longValuesOK  = false;

    PG_RETURN_VOID();
}

/**
* helper function to guide a the child explorations
* that is what makes the use of labels unnecessary
**/
static inline int
kmer_node_for_level(const Kmer *k, int level)
{
    if (level >= k->length)
        return 4;  // end
    char c = kmer_get_base(k, level);
    // based on this char we now wich child to explore
    switch (c)
    {
    case 'A': return 0;
    case 'C': return 1;
    case 'G': return 2;
    case 'T': return 3;
    default:
        ereport(ERROR,
                (errcode(ERRCODE_DATA_EXCEPTION),
                  errmsg("invalid base '%c' in kmer", c)));
        return 0;
    }
}


Datum
spg_kmer_choose(PG_FUNCTION_ARGS)
{
    spgChooseIn  *in  = (spgChooseIn *) PG_GETARG_POINTER(0);
    spgChooseOut *out = (spgChooseOut *) PG_GETARG_POINTER(1);

    // extract the kmer value
    Kmer *k = (Kmer *) PG_DETOAST_DATUM(in->datum);

    int nodeN = kmer_node_for_level(k, in->level); //determine which child to visit


    if (in->nNodes > 0 && nodeN >= in->nNodes) // avoid accessing an index that dosen't exist
        nodeN = in->nNodes - 1;

    /* We match an existing node */
    out->resultType = spgMatchNode;

    out->result.matchNode.nodeN     = nodeN;
    out->result.matchNode.levelAdd  = 1;
    out->result.matchNode.restDatum = in->datum;

    PG_RETURN_VOID();
}


Datum
spg_kmer_picksplit(PG_FUNCTION_ARGS)
{
    spgPickSplitIn  *in  = (spgPickSplitIn *) PG_GETARG_POINTER(0);
    spgPickSplitOut *out = (spgPickSplitOut *) PG_GETARG_POINTER(1);

    int i;

    //we don't store prefixe since each level corrspond to the base at position level
    out->hasPrefix   = false;
    out->prefixDatum = (Datum) 0;

     //fixe pattern no need for a label
    out->nNodes     = 5;
    out->nodeLabels = NULL;

    //allocate output tables
    out->mapTuplesToNodes =
        (int *) palloc(sizeof(int) * in->nTuples);
    out->leafTupleDatums  =
        (Datum *) palloc(sizeof(Datum) * in->nTuples);

    for (i = 0; i < in->nTuples; i++)
    {
        Kmer *k = (Kmer *) PG_DETOAST_DATUM(in->datums[i]);
        int   nodeN = kmer_node_for_level(k, in->level);

        // should never happend but just in case
        if (nodeN < 0 || nodeN >= out->nNodes)
            ereport(ERROR,
                    (errcode(ERRCODE_DATA_EXCEPTION),
                     errmsg("spgist kmer picksplit: invalid node index %d", nodeN)));

        // tells in which child this tuple gies
        out->mapTuplesToNodes[i] = nodeN;
        // kmer stored as a leaf
        out->leafTupleDatums[i]  = in->datums[i];
    }

    PG_RETURN_VOID();
}


static inline int
qkmer_length_internal_spg(const QKmer *q)
{
    Size size   = VARSIZE_ANY(q);
    Size header = offsetof(QKmer, data);

    if (size < header)
        ereport(ERROR,
                (errcode(ERRCODE_DATA_CORRUPTED),
                 errmsg("qkmer value is corrupted")));
    return (int) (size - header);
}

// IUPAC qkmer code
static inline bool
qbase_matches_spg(char q, char base)
{
    switch (q)
    {
        case 'A': return base == 'A';
        case 'C': return base == 'C';
        case 'G': return base == 'G';
        case 'T': return base == 'T';

        case 'N': return (base == 'A' || base == 'C' || base == 'G' || base == 'T');
        case 'R': return (base == 'A' || base == 'G');
        case 'Y': return (base == 'C' || base == 'T');
        case 'S': return (base == 'G' || base == 'C');
        case 'W': return (base == 'A' || base == 'T');
        case 'K': return (base == 'G' || base == 'T');
        case 'M': return (base == 'A' || base == 'C');
        case 'B': return (base == 'C' || base == 'G' || base == 'T');
        case 'D': return (base == 'A' || base == 'G' || base == 'T');
        case 'H': return (base == 'A' || base == 'C' || base == 'T');
        case 'V': return (base == 'A' || base == 'C' || base == 'G');

        default:
            ereport(ERROR,
                    (errcode(ERRCODE_DATA_CORRUPTED),
                     errmsg("qkmer contains invalid IUPAC code '%c'", q)));
            return false;
    }
}

// map index child
static inline char
child_index_to_base(int nodeN)
{
    switch (nodeN)
    {
        case 0: return 'A';
        case 1: return 'C';
        case 2: return 'G';
        case 3: return 'T';
        default: return '\0';  //end
    }
}


Datum
spg_kmer_inner_consistent(PG_FUNCTION_ARGS)
{
    spgInnerConsistentIn  *in  = (spgInnerConsistentIn *) PG_GETARG_POINTER(0);
    spgInnerConsistentOut *out = (spgInnerConsistentOut *) PG_GETARG_POINTER(1);
    elog(WARNING, "inner_consistent called: level=%d, nNodes=%d, nkeys=%d",
         in->level, in->nNodes, in->nkeys);
    
    int nVisit = 0;

    // not used
    out->reconstructedValues = NULL;
    out->traversalValues     = NULL;
    out->distances           = NULL;

    // allcate to visit potential childeren
    out->nodeNumbers = (int *) palloc(sizeof(int) * in->nNodes); //worst case visit all childeren so e allocate nNode * int
    out->levelAdds   = (int *) palloc(sizeof(int) * in->nNodes);

    //without search key (WHERE) we cann't prune anything so we just visit all
    if (in->nkeys == 0|| in->allTheSame)
    {
        for (int i = 0; i < in->nNodes; i++)
        {
            out->nodeNumbers[nVisit] = i;
            out->levelAdds[nVisit]   = 1;  // we go down a level
            nVisit++;
        }
        out->nNodes = nVisit;
        PG_RETURN_VOID();
    }

    //  we just prune using the first cond key : WHERE <cond1> AND <cond2>. to guide the pruning
    // (can be optimized to prune more)
    // with = we prune more agresively because there is only one possible path at each level
    ScanKey        key      = &in->scankeys[0];
    StrategyNumber strategy = key->sk_strategy; // get the strategy number of the operator

     //strat k = query_kmer (handle WHERE k = ...) type of queries
    if (strategy == BTEqualStrategyNumber) // if operator is =
    {
        Kmer *q = (Kmer *) PG_DETOAST_DATUM(key->sk_argument);
        int   qlen = q->length;

        int nodeN;

        if (in->level < qlen)
        {
            nodeN = kmer_node_for_level(q, in->level); //gives us the exact child to follow so we don't visit all childeren nodes
        }
        else
        {
            //we have consumed everything : end
            nodeN = 4;
        }

        if (nodeN >= 0 && nodeN < in->nNodes)
        {
            out->nodeNumbers[nVisit] = nodeN;
            out->levelAdds[nVisit]   = 1;
            nVisit++;
        }

        out->nNodes = nVisit;
    }

     //préfixe k starts_with prefix ^@ operator
    // less agressive pruning due to the fact that we have to return a set with multiple branches
    else if (strategy == KMER_PREFIX_CONTAINS_STRATEGY	)
    {
        Kmer *prefix = (Kmer *) PG_DETOAST_DATUM(key->sk_argument);
        int   plen   = prefix->length;

        if (in->level < plen)
        {
            //while we have not explored all the prefixe on the path we continue on the correponding base
            int nodeN = kmer_node_for_level(prefix, in->level); // same as = operator but until we reach prefix lenght then we explore all

            if (nodeN >= 0 && nodeN < in->nNodes)
            {
                out->nodeNumbers[nVisit] = nodeN;
                out->levelAdds[nVisit]   = 1;
                nVisit++;
            }
        }
        else
        {
             // entire prefixe on the path we followed all the base
            for (int i = 0; i < in->nNodes; i++) // since we aleardy mateched the prefixe, now we just have from there visit everything
            {
                out->nodeNumbers[nVisit] = i;
                out->levelAdds[nVisit]   = 1;
                nVisit++;
            }
        }

        out->nNodes = nVisit;
    }
    // qkmer
	else if (strategy == KMER_QKMER_CONTAINS_STRATEGY)
	{
    	QKmer *pattern;
    	int    plen;
    	int    level;
    	int    i;

   		 /* 1) Récupérer le qkmer de la requête */
   		pattern = (QKmer *) PG_DETOAST_DATUM(key->sk_argument);
    	plen    = qkmer_length_internal_spg(pattern);
    	level   = in->level;


    	if (level >= plen)
    	{
        	for (i = 0; i < in->nNodes; i++)
        	{
        	    out->nodeNumbers[nVisit] = i;
        	    out->levelAdds[nVisit]   = 1;
        	    nVisit++;
        	}
    	}
    	else
    	{
      		char q;

        	q = pattern->data[level];


        	for (i = 0; i < in->nNodes; i++)
        	{
           		int  child = i;
            	char base  = child_index_to_base(child);

            	if (base == '\0')
            	{

                	out->nodeNumbers[nVisit] = child;
                	out->levelAdds[nVisit]   = 1;
                	nVisit++;
            	}
            	else
            	{

                	if (qbase_matches_spg(q, base))
                	{
                    	out->nodeNumbers[nVisit] = child;
                    	out->levelAdds[nVisit]   = 1;
                    	nVisit++;
                	}
            	}
        	}
    	}

    	out->nNodes = nVisit;
	}

    //evrything else  we just visit all childeren
    else
    {
        for (int i = 0; i < in->nNodes; i++) //just visits verything in a bfs way
        {
            out->nodeNumbers[nVisit] = i;
            out->levelAdds[nVisit]   = 1;
            nVisit++;
        }
        out->nNodes = nVisit;
    }

    PG_RETURN_VOID();
}

//small helper (strcit equality)
static bool
kmer_equal_internal(const Kmer *a, const Kmer *b)
{
    if (a->length != b->length)
        return false;

    for (int i = 0; i < a->length; i++)
    {
        if (kmer_get_base(a, i) != kmer_get_base(b, i))
            return false;
    }
    return true;
}

//helper
static bool
kmer_starts_with_internal(const Kmer *prefix, const Kmer *value)
{
    int np = prefix->length;
    int nv = value->length;

    if (np > nv)
        ereport(ERROR,
                (errcode(ERRCODE_DATA_EXCEPTION),
                 errmsg("starts_with: prefix length %d exceeds kmer length %d",
                        np, nv)));

    for (int i = 0; i < np; i++)
    {
        if (kmer_get_base(prefix, i) != kmer_get_base(value, i))
            return false;
    }
    return true;
}

//applies all condition : if one fails the leaf is rejected else accepted
Datum
spg_kmer_leaf_consistent(PG_FUNCTION_ARGS)
{
    spgLeafConsistentIn  *in  = (spgLeafConsistentIn *) PG_GETARG_POINTER(0);
    spgLeafConsistentOut *out = (spgLeafConsistentOut *) PG_GETARG_POINTER(1);

    bool res = true;
    int  i;

    //leaf value
    out->leafValue        = in->leafDatum;
    out->recheck          = false;
    out->recheckDistances = false;

    // no key -> evrything matches
    if (in->nkeys == 0)
        PG_RETURN_BOOL(true);

    //kmer of the leaf
    Kmer *leaf = (Kmer *) PG_DETOAST_DATUM(in->leafDatum);

    for (i = 0; i < in->nkeys; i++)
    {
        ScanKey        key      = &in->scankeys[i];
        StrategyNumber strategy = key->sk_strategy;

        if (strategy == BTEqualStrategyNumber)
        {
            Kmer *query = (Kmer *) PG_DETOAST_DATUM(key->sk_argument);

            if (!kmer_equal_internal(leaf, query))
            {
                res = false;
                break;
            }
        }
        else if (strategy == KMER_PREFIX_CONTAINS_STRATEGY)
        {
            Kmer *prefix = (Kmer *) PG_DETOAST_DATUM(key->sk_argument);

            if (!kmer_starts_with_internal(prefix, leaf))
            {
                res = false;
                break;
            }
        }
        else if (strategy == KMER_QKMER_CONTAINS_STRATEGY)
        {


            QKmer *pattern   = (QKmer *) PG_DETOAST_DATUM(key->sk_argument);
            int    plen      = qkmer_length_internal_spg(pattern);
            int    klen      = leaf->length;
            int    pos;
            bool   ok = true;

            // same lenght
            if (plen != klen)
            {
                res = false;
                break;
            }

            for (pos = 0; pos < plen; pos++)
            {
                char q = pattern->data[pos];   //iupac
                char b = kmer_get_base(leaf, pos);  //ACGT

                if (!qbase_matches_spg(q, b))
                {
                    ok = false;
                    break;
                }
            }

            if (!ok)
            {
                res = false;
                break;
            }
        }
        else
        {
            out->recheck = true;
        }
    }

    PG_RETURN_BOOL(res);
}

