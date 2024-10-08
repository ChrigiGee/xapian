/** @file
 * @brief A TermList in a honey database.
 */
/* Copyright (C) 2007,2008,2009,2010,2011,2024 Olly Betts
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#ifndef XAPIAN_INCLUDED_HONEY_TERMLIST_H
#define XAPIAN_INCLUDED_HONEY_TERMLIST_H

#include <algorithm>
#include <string>

#include "xapian/intrusive_ptr.h"
#include <xapian/positioniterator.h>
#include <xapian/types.h>

namespace Xapian {
    namespace Internal {
	class ExpandStats;
    }
}

#include "honey_database.h"
#include "api/termlist.h"
#include "honey_table.h"

/// A TermList in a honey database.
class HoneyTermList : public TermList {
    /// Don't allow assignment.
    void operator=(const HoneyTermList&);

    /// Don't allow copying.
    HoneyTermList(const HoneyTermList&);

    /// The database we're reading data from.
    Xapian::Internal::intrusive_ptr<const HoneyDatabase> db;

    /// The document id that this TermList is for.
    Xapian::docid did;

    /// The length of document @a did.
    Xapian::termcount doclen;

    /// The number of entries in this termlist.
    Xapian::termcount termlist_size;

    /// The tag value from the termlist table which holds the encoded termlist.
    std::string data;

    /** Current position with the encoded tag value held in @a data.
     *
     *  If we've iterated to the end of the list, this gets set to NULL.
     */
    const char* pos;

    /// Pointer to the end of the encoded tag value.
    const char* end;

    /// The wdf for the term at the current position.
    Xapian::termcount current_wdf;

    /** The term frequency for the term at the current position.
     *
     *  This will have the value 0 if the term frequency has not yet been
     *  looked up in the database (so it needs to be mutable).
     */
    mutable Xapian::doccount current_termfreq;

  public:
    /// Create a new HoneyTermList object for document @a did_ in DB @a db_
    HoneyTermList(const HoneyDatabase* db_, Xapian::docid did_);

    /** Return the length of this document.
     *
     *  This is a non-virtual method, used by HoneyDatabase.
     */
    Xapian::termcount get_doclength() const {
	return doclen;
    }

    /** Return the number of entries in this termlist.
     *
     *  This is a non-virtual method, used by HoneyDatabase.
     */
    Xapian::termcount size() const {
	return termlist_size;
    }

    /** Return the number of unique terms.
     *
     *  This is a non-virtual method, used by HoneyDatabase.
     */
    Xapian::termcount get_unique_terms() const {
	// We approximate get_unique_terms() by the length of the termlist
	// (which counts boolean terms too) but clamp the result to be no
	// larger than the document length.
	return std::min(termlist_size, doclen);
    }

    /** Return approximate size of this termlist.
     *
     *  For a HoneyTermList, this value will always be exact.
     */
    Xapian::termcount get_approx_size() const;

    /// Collate weighting information for the current term.
    void accumulate_stats(Xapian::Internal::ExpandStats& stats) const;

    /// Return the wdf for the term at the current position.
    Xapian::termcount get_wdf() const;

    /** Return the term frequency for the term at the current position.
     *
     *  In order to be able to support updating databases efficiently, we can't
     *  store this value in the termlist table, so it has to be read from the
     *  postlist table, which is relatively expensive (compared to reading the
     *  wdf for example).
     */
    Xapian::doccount get_termfreq() const;

    /** Advance the current position to the next term in the termlist.
     *
     *  The list starts before the first term in the list, so next(), skip_to()
     *  or check() must be called before any methods which need the context of
     *  the current position.
     *
     *  @return Always returns 0 for a HoneyTermList.
     */
    TermList* next();

    TermList* skip_to(std::string_view term);

    /// Return the length of the position list for the current position.
    Xapian::termcount positionlist_count() const;

    /// Return a PositionIterator for the current position.
    PositionList* positionlist_begin() const;
};

#endif // XAPIAN_INCLUDED_HONEY_TERMLIST_H
