/** @file
 * @brief Combine subqueries, weighting as if they are synonyms
 */
/* Copyright 2007,2009 Lemur Consulting Ltd
 * Copyright 2009,2011,2014,2016,2017,2018,2024 Olly Betts
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301
 * USA
 */

#include <config.h>

#include "synonympostlist.h"

#include "debuglog.h"
#include "omassert.h"
#include "postlisttree.h"

using namespace std;

SynonymPostList::~SynonymPostList()
{
    delete wt;
}

void
SynonymPostList::set_weight(const Xapian::Weight * wt_)
{
    delete wt;
    wt = wt_;
    want_wdf = wt->get_sumpart_needs_wdf_();
    want_wdfdocmax = wt->get_sumpart_needs_wdfdocmax_();
}

PostList *
SynonymPostList::next(double w_min)
{
    LOGCALL(MATCH, PostList *, "SynonymPostList::next", w_min);
    (void)w_min;
    RETURN(WrapperPostList::next(0.0));
}

PostList *
SynonymPostList::skip_to(Xapian::docid did, double w_min)
{
    LOGCALL(MATCH, PostList *, "SynonymPostList::skip_to", did | w_min);
    (void)w_min;
    RETURN(WrapperPostList::skip_to(did, 0.0));
}

double
SynonymPostList::get_weight(Xapian::termcount doclen,
			    Xapian::termcount unique_terms,
			    Xapian::termcount wdfdocmax) const
{
    LOGCALL(MATCH, double, "SynonymPostList::get_weight", doclen | unique_terms);
    Xapian::termcount wdf = 0;
    if (want_wdf) {
	wdf = WrapperPostList::get_wdf();
	if (needs_doclen) {
	    // The wdf for a synonym is approximated and in some cases it could
	    // exceed the document length.  For example, this can currently
	    // occur if the query below OP_SYNONYM contains a term more than
	    // once as the wdf of each occurrence is summed.
	    //
	    // This is unhelpful since it's reasonable for weighting algorithms
	    // to optimise by assuming that get_wdf() will never return more
	    // than doclen, since doclen is the sum of the wdfs.
	    //
	    // If the weighting scheme doesn't request the document length then
	    // it can't be making this assumption, so we simply clamp the wdf
	    // value to doclen if both are requested, since the clamping is
	    // cheap in this case as we already have both values.
	    if (wdf > doclen) wdf = doclen;
	}
    }
    if (want_wdfdocmax) {
	// FIXME: Can we avoid this?
	if (doclen == 0) {
	    doclen = pltree->get_doclength(pl->get_docid());
	}
	wdfdocmax = doclen;
    }
    RETURN(wt->get_sumpart(wdf, doclen, unique_terms, wdfdocmax));
}

double
SynonymPostList::recalc_maxweight()
{
    LOGCALL(MATCH, double, "SynonymPostList::recalc_maxweight", NO_ARGS);
    RETURN(wt->get_maxpart());
}

Xapian::termcount
SynonymPostList::count_matching_subqs() const
{
    return 1;
}

string
SynonymPostList::get_description() const
{
    string desc = "SynonymPostList(";
    desc += pl->get_description();
    desc += ')';
    return desc;
}
