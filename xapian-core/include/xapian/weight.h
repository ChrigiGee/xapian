/** @file
 * @brief Weighting scheme API.
 */
/* Copyright (C) 2004-2024 Olly Betts
 * Copyright (C) 2009 Lemur Consulting Ltd
 * Copyright (C) 2013,2014 Aarsh Shah
 * Copyright (C) 2016,2017 Vivek Pal
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#ifndef XAPIAN_INCLUDED_WEIGHT_H
#define XAPIAN_INCLUDED_WEIGHT_H

#include <string>

#include <xapian/database.h>
#include <xapian/deprecated.h>
#include <xapian/registry.h>
#include <xapian/types.h>
#include <xapian/visibility.h>

namespace Xapian {

/** Abstract base class for weighting schemes. */
class XAPIAN_VISIBILITY_DEFAULT Weight {
  protected:
    /// Stats which the weighting scheme can use (see @a need_stat()).
    typedef enum {
	/// Number of documents in the collection.
	COLLECTION_SIZE = 0,
	/// Number of documents in the RSet.
	RSET_SIZE = 0,
	/// Average length of documents in the collection.
	AVERAGE_LENGTH = 4,
	/// How many documents the current term is in.
	TERMFREQ = 1,
	/// How many documents in the RSet the current term is in.
	RELTERMFREQ = 1,
	/// Sum of wqf for terms in the query.
	QUERY_LENGTH = 0,
	/// Within-query-frequency of the current term.
	WQF = 0,
	/// Within-document-frequency of the current term in the current document.
	WDF = 2,
	/// Length of the current document (sum wdf).
	DOC_LENGTH = 8,
	/** Lower bound on (non-zero) document lengths.
	 *  This bound is for the current shard and is suitable for using to
	 *  calculate upper bounds to return from get_maxpart() and
	 *  get_maxextra().
	 */
	DOC_LENGTH_MIN = 16,
	/** Upper bound on document lengths.
	 *  This bound is for the current shard and is suitable for using to
	 *  calculate upper bounds to return from get_maxpart() and
	 *  get_maxextra().  If you need a bound for calculating a returned
	 *  weight from get_sumpart() or get_sumextra() then you should use
	 *  DB_DOC_LENGTH_MIN instead.
	 */
	DOC_LENGTH_MAX = 32,
	/** Upper bound on wdf.
	 *  This bound is for the current shard and is suitable for using to
	 *  calculate upper bounds to return from get_maxpart() and
	 *  get_maxextra().  If you need a bound for calculating a returned
	 *  weight from get_sumpart() or get_sumextra() then you should use
	 *  DB_DOC_LENGTH_MAX instead.
	 */
	WDF_MAX = 64,
	/// Sum of wdf over the whole collection for the current term.
	COLLECTION_FREQ = 1,
	/// Number of unique terms in the current document.
	UNIQUE_TERMS = 128,
	/** Sum of lengths of all documents in the collection.
	 *  This gives the total number of term occurrences.
	 */
	TOTAL_LENGTH = 256,
	/** Maximum wdf in the current document.
	 *
	 *  @since 1.5.0
	 */
	WDF_DOC_MAX = 512,
	/** Lower bound on number of unique terms in a document.
	 *  This bound is for the current shard and is suitable for using to
	 *  calculate upper bounds to return from get_maxpart() and
	 *  get_maxextra().  If you need a bound for calculating a returned
	 *  weight from get_sumpart() or get_sumextra() then you should use
	 *  DB_UNIQUE_TERMS_MIN instead.
	 *
	 *  @since 1.5.0
	 */
	UNIQUE_TERMS_MIN = 1024,
	/** Upper bound on number of unique terms in a document.
	 *  This bound is for the current shard and is suitable for using to
	 *  calculate upper bounds to return from get_maxpart() and
	 *  get_maxextra().  If you need a bound for calculating a returned
	 *  weight from get_sumpart() or get_sumextra() then you should use
	 *  DB_UNIQUE_TERMS_MAX instead.
	 *
	 *  @since 1.5.0
	 */
	UNIQUE_TERMS_MAX = 2048,
	/** Lower bound on (non-zero) document lengths.
	 *  This is a suitable bound for calculating a returned weight from
	 *  get_sumpart() or get_sumextra().
	 *
	 *  @since 1.5.0
	 */
	DB_DOC_LENGTH_MIN = 4096,
	/** Upper bound on document lengths.
	 *  This is a suitable bound for calculating a returned weight from
	 *  get_sumpart() or get_sumextra().
	 *
	 *  @since 1.5.0
	 */
	DB_DOC_LENGTH_MAX = 8192,
	/** Lower bound on number of unique terms in a document.
	 *  This is a suitable bound for calculating a returned weight from
	 *  get_sumpart() or get_sumextra();
	 *
	 *  @since 1.5.0
	 */
	DB_UNIQUE_TERMS_MIN = 16384,
	/** Upper bound on number of unique terms in a document.
	 *  This is a suitable bound for calculating a returned weight from
	 *  get_sumpart() or get_sumextra();
	 *
	 *  @since 1.5.0
	 */
	DB_UNIQUE_TERMS_MAX = 32768,
	/** Upper bound on wdf of this term.
	 *  This is a suitable bound for calculating a returned weight from
	 *  get_sumpart().
	 *
	 *  @since 1.5.0
	 */
	DB_WDF_MAX = 65536,
	/** @private @internal Flag only set for BoolWeight.
	 *  This allows us to efficiently indentify BoolWeight objects.
	 */
	IS_BOOLWEIGHT_ = static_cast<int>(0x80000000)
    } stat_flags;

    /** Tell Xapian that your subclass will want a particular statistic.
     *
     *  Some of the statistics can be costly to fetch or calculate, so
     *  Xapian needs to know which are actually going to be used.  You
     *  should call need_stat() from your constructor for each statistic
     *  needed by the weighting scheme you are implementing (possibly
     *  conditional on the values of parameters of the weighting scheme).
     *
     *  Some of the statistics are currently available by default and their
     *  constants above have value 0 (e.g. COLLECTION_SIZE).  You should
     *  still call need_stat() for these (the compiler should optimise away
     *  these calls and any conditional checks for them).
     *
     *  Some statistics are currently fetched together and so their constants
     *  have the same numeric value - if you need more than one of these
     *  statistics you should call need_stat() for each one.  The compiler
     *  should optimise this too.
     *
     *  Prior to 1.5.0, it was assumed that if get_maxextra() returned
     *  a non-zero value then get_sumextra() needed the document length even if
     *  need(DOC_LENGTH) wasn't called - the logic was that get_sumextra() could
     *  only return a constant value if it didn't use the document length.
     *  However, this is no longer valid since it can also use the number of
     *  unique terms in the document, so now you need to specify explicitly.
     *
     * @param flag  The stat_flags value for a required statistic.
     */
    void need_stat(stat_flags flag) {
	stats_needed = stat_flags(stats_needed | flag);
    }

    /** Allow the subclass to perform any initialisation it needs to.
     *
     *  @param factor	  Any scaling factor (e.g. from OP_SCALE_WEIGHT).
     *			  If the Weight object is for the term-independent
     *			  weight supplied by get_sumextra()/get_maxextra(),
     *			  then init(0.0) is called (starting from Xapian
     *			  1.2.11 and 1.3.1 - earlier versions failed to
     *			  call init() for such Weight objects).
     */
    virtual void init(double factor) = 0;

  private:
    /// Don't allow assignment.
    void operator=(const Weight &);

    /// A bitmask of the statistics this weighting scheme needs.
    stat_flags stats_needed;

    /// The number of documents in the collection.
    Xapian::doccount collection_size_;

    /// The number of documents marked as relevant.
    Xapian::doccount rset_size_;

    /// The average length of a document in the collection.
    Xapian::doclength average_length_;

    /// The number of documents which this term indexes.
    Xapian::doccount termfreq_;

    // The collection frequency of the term.
    Xapian::termcount collectionfreq_;

    /// The number of relevant documents which this term indexes.
    Xapian::doccount reltermfreq_;

    /// The length of the query.
    Xapian::termcount query_length_;

    /// The within-query-frequency of this term.
    Xapian::termcount wqf_;

    /// A lower bound on the minimum length of any document in the shard.
    Xapian::termcount doclength_lower_bound_;

    /// An upper bound on the maximum length of any document in the shard.
    Xapian::termcount doclength_upper_bound_;

    /// An upper bound on the wdf of this term in the shard.
    Xapian::termcount wdf_upper_bound_;

    /// Total length of all documents in the collection.
    Xapian::totallength total_length_;

    /** A lower bound on the number of unique terms in any document in the
     *  shard.
     */
    Xapian::termcount unique_terms_lower_bound_;

    /** An upper bound on the number of unique terms in any document in the
     *  shard.
     */
    Xapian::termcount unique_terms_upper_bound_;

    /// A lower bound on the minimum length of any document in the database.
    Xapian::termcount db_doclength_lower_bound_;

    /// An upper bound on the maximum length of any document in the database.
    Xapian::termcount db_doclength_upper_bound_;

    /// An upper bound on the wdf of this term in the database.
    Xapian::termcount db_wdf_upper_bound_;

    /** A lower bound on the number of unique terms in any document in the
     *  database.
     */
    Xapian::termcount db_unique_terms_lower_bound_;

    /** An upper bound on the number of unique terms in any document in the
     *  database.
     */
    Xapian::termcount db_unique_terms_upper_bound_;

  public:

    /// Default constructor, needed by subclass constructors.
    Weight() : stats_needed() { }

    class Internal;

    /** Virtual destructor, because we have virtual methods. */
    virtual ~Weight();

    /** Clone this object.
     *
     *  This method allocates and returns a copy of the object it is called on.
     *
     *  If your subclass is called FooWeight and has parameters a and b, then
     *  you would implement FooWeight::clone() like so:
     *
     *  FooWeight * FooWeight::clone() const { return new FooWeight(a, b); }
     *
     *  Note that the returned object will be deallocated by Xapian after use
     *  with "delete".  If you want to handle the deletion in a special way
     *  (for example when wrapping the Xapian API for use from another
     *  language) then you can define a static <code>operator delete</code>
     *  method in your subclass as shown here:
     *  https://trac.xapian.org/ticket/554#comment:1
     */
    virtual Weight * clone() const = 0;

    /** Return the name of this weighting scheme, e.g. "bm25+".
     *
     *  This is the name that the weighting scheme gets registered under
     *  when passed to Xapian:Registry::register_weighting_scheme().
     *
     *  As a result:
     *
     *  * this is the name that needs to be used in Weight::create() to
     *    create a Weight object from a human-readable string description.
     *
     *  * it is also used by the remote backend where it is sent (along with
     *    the serialised parameters) to the remote server so that it knows
     *    which class to create.
     *
     *  For 1.4.x and earlier we recommended returning the full
     *  namespace-qualified name of your class here, but now we recommend
     *  returning a just the name in lower case, e.g. "foo" instead of
     *  "FooWeight", "bm25+" instead of "Xapian::BM25PlusWeight".
     *
     *  If you don't want to support creation via Weight::create() or the
     *  remote backend, you can use the default implementation which simply
     *  returns an empty string.
     */
    virtual std::string name() const;

    /** Return this object's parameters serialised as a single string.
     *
     *  If you don't want to support the remote backend, you can use the
     *  default implementation which simply throws Xapian::UnimplementedError.
     */
    virtual std::string serialise() const;

    /** Unserialise parameters.
     *
     *  This method unserialises parameters serialised by the @a serialise()
     *  method and allocates and returns a new object initialised with them.
     *
     *  If you don't want to support the remote backend, you can use the
     *  default implementation which simply throws Xapian::UnimplementedError.
     *
     *  Note that the returned object will be deallocated by Xapian after use
     *  with "delete".  If you want to handle the deletion in a special way
     *  (for example when wrapping the Xapian API for use from another
     *  language) then you can define a static <code>operator delete</code>
     *  method in your subclass as shown here:
     *  https://trac.xapian.org/ticket/554#comment:1
     *
     *  @param serialised	A string containing the serialised parameters.
     */
    virtual Weight * unserialise(const std::string & serialised) const;

    /** Calculate the weight contribution for this object's term to a document.
     *
     *  The parameters give information about the document which may be used
     *  in the calculations:
     *
     *  @param wdf    The within document frequency of the term in the document.
     *		      You need to call need_stat(WDF) if you use this value.
     *  @param doclen The document's length (unnormalised).
     *		      You need to call need_stat(DOC_LENGTH) if you use this
     *		      value.
     *  @param uniqterms
     *		      Number of unique terms in the document.
     *		      You need to call need_stat(UNIQUE_TERMS) if you use this
     *		      value.
     *  @param wdfdocmax
     *		      Maximum wdf value in the document.
     *		      You need to call need_stat(WDF_DOC_MAX) if you use this
     *		      value.
     *
     *	You can rely of wdf <= doclen if you call both need_stat(WDF) and
     *	need_stat(DOC_LENGTH) - this is trivially true for terms, but Xapian
     *	also ensure it's true for OP_SYNONYM, where the wdf is approximated.
     */
    virtual double get_sumpart(Xapian::termcount wdf,
			       Xapian::termcount doclen,
			       Xapian::termcount uniqterms,
			       Xapian::termcount wdfdocmax) const = 0;

    /** Return an upper bound on what get_sumpart() can return for any document.
     *
     *  This information is used by the matcher to perform various
     *  optimisations, so strive to make the bound as tight as possible.
     */
    virtual double get_maxpart() const = 0;

    /** Calculate the term-independent weight component for a document.
     *
     *  The default implementation always returns 0 (in Xapian < 1.5.0 this
     *  was a pure virtual method).
     *
     *  The parameter gives information about the document which may be used
     *  in the calculations:
     *
     *  @param doclen The document's length (unnormalised).
     *  @param uniqterms The number of unique terms in the document.
     */
    virtual double get_sumextra(Xapian::termcount doclen,
				Xapian::termcount uniqterms,
				Xapian::termcount wdfdocmax) const;

    /** Return an upper bound on what get_sumextra() can return for any
     *  document.
     *
     *  The default implementation always returns 0 (in Xapian < 1.5.0 this
     *  was a pure virtual method).
     *
     *  This information is used by the matcher to perform various
     *  optimisations, so strive to make the bound as tight as possible.
     */
    virtual double get_maxextra() const;

    /** @private @internal Initialise this object to calculate weights for term
     *  @a term.
     *
     *  @param stats	  Source of statistics.
     *  @param query_len_ Query length.
     *  @param term	  The term for the new object.
     *  @param wqf_	  The within-query-frequency of @a term.
     *  @param factor	  Any scaling factor (e.g. from OP_SCALE_WEIGHT).
     *  @param postlist   Pointer to a LeafPostList for the term (cast to void*
     *			  to avoid needing to forward declare class
     *			  LeafPostList in public API headers) which can be used
     *			  to get wdf upper bound
     */
    XAPIAN_VISIBILITY_INTERNAL
    void init_(const Internal & stats, Xapian::termcount query_len_,
	       const std::string & term, Xapian::termcount wqf_,
	       double factor,
	       const Xapian::Database::Internal* shard,
	       void* postlist);

    /** @private @internal Initialise this object to calculate weights for a
     *  synonym.
     *
     *  @param stats	   Source of statistics.
     *  @param query_len_  Query length.
     *  @param factor	   Any scaling factor (e.g. from OP_SCALE_WEIGHT).
     *  @param termfreq    The termfreq to use.
     *  @param reltermfreq The reltermfreq to use.
     *  @param collection_freq The collection frequency to use.
     */
    XAPIAN_VISIBILITY_INTERNAL
    void init_(const Internal & stats, Xapian::termcount query_len_,
	       double factor, Xapian::doccount termfreq,
	       Xapian::doccount reltermfreq, Xapian::termcount collection_freq,
	       const Xapian::Database::Internal* shard);

    /** @private @internal Initialise this object to calculate the extra weight
     *  component.
     *
     *  @param stats	  Source of statistics.
     *  @param query_len_ Query length.
     */
    XAPIAN_VISIBILITY_INTERNAL
    void init_(const Internal & stats, Xapian::termcount query_len_,
	       const Xapian::Database::Internal* shard);

    /** @private @internal Return true if the document length is needed.
     *
     *  If this method returns true, then the document length will be fetched
     *  and passed to @a get_sumpart().  Otherwise 0 may be passed for the
     *  document length.
     */
    bool get_sumpart_needs_doclength_() const {
	return stats_needed & DOC_LENGTH;
    }

    /** @private @internal Return true if the WDF is needed.
     *
     *  If this method returns true, then the WDF will be fetched and passed to
     *  @a get_sumpart().  Otherwise 0 may be passed for the wdf.
     */
    bool get_sumpart_needs_wdf_() const {
	return stats_needed & WDF;
    }

    /** @private @internal Return true if the number of unique terms is needed.
     *
     *  If this method returns true, then the number of unique terms will be
     *  fetched and passed to @a get_sumpart().  Otherwise 0 may be passed for
     *  the number of unique terms.
     */
    bool get_sumpart_needs_uniqueterms_() const {
	return stats_needed & UNIQUE_TERMS;
    }

    /** Return the appropriate weighting scheme object.
     *
     *  @param scheme	the string containing a weighting scheme name and may
     *			also contain the parameters required by that weighting
     *			scheme. E.g. "bm25 1.0 0.8"
     *  @param reg	Xapian::Registry object to allow users to add their own
     *			custom weighting schemes (default: standard registry).
     *
     *  @since 1.5.0
     */
    static const Weight * create(const std::string & scheme,
				 const Registry & reg = Registry());

    /** Create from a human-readable parameter string.
     *
     * @param params	string containing weighting scheme parameter values.
     *
     *  @since 1.5.0
     */
    virtual Weight * create_from_parameters(const char * params) const;

    /// @private @internal Test if this is a BoolWeight object.
    bool is_bool_weight_() const {
	// We use a special flag bit to make this check efficient.  Note we
	// can't use (get_maxpart() == 0.0) since that's not required to work
	// without init() having been called.
	return stats_needed & IS_BOOLWEIGHT_;
    }

    /** @private @internal Return true if the max WDF of document is needed.
     *
     *  If this method returns true, then the max WDF will be
     *  fetched and passed to @a get_sumpart().  Otherwise 0 may be passed for
     *  the max wdf.
     */
    bool get_sumpart_needs_wdfdocmax_() const {
	return stats_needed & WDF_DOC_MAX;
    }

  protected:
    /** Don't allow copying.
     *
     *  This would ideally be private, but that causes a compilation error
     *  with GCC 4.1 (which appears to be a bug).
     */
    XAPIAN_VISIBILITY_INTERNAL
    Weight(const Weight &);

    /// The number of documents in the collection.
    Xapian::doccount get_collection_size() const { return collection_size_; }

    /// The number of documents marked as relevant.
    Xapian::doccount get_rset_size() const { return rset_size_; }

    /// The average length of a document in the collection.
    Xapian::doclength get_average_length() const { return average_length_; }

    /// The number of documents which this term indexes.
    Xapian::doccount get_termfreq() const { return termfreq_; }

    /// The number of relevant documents which this term indexes.
    Xapian::doccount get_reltermfreq() const { return reltermfreq_; }

    /// The collection frequency of the term.
    Xapian::termcount get_collection_freq() const { return collectionfreq_; }

    /// The length of the query.
    Xapian::termcount get_query_length() const { return query_length_; }

    /// The within-query-frequency of this term.
    Xapian::termcount get_wqf() const { return wqf_; }

    /** An upper bound on the maximum length of any document in the shard.
     *
     *  This should only be used by get_maxpart() and get_maxextra().
     */
    Xapian::termcount get_doclength_upper_bound() const {
	return doclength_upper_bound_;
    }

    /** A lower bound on the minimum length of any document in the shard.
     *
     *  This bound does not include any zero-length documents.
     *
     *  This should only be used by get_maxpart() and get_maxextra().
     */
    Xapian::termcount get_doclength_lower_bound() const {
	return doclength_lower_bound_;
    }

    /** An upper bound on the wdf of this term in the shard.
     *
     *  This should only be used by get_maxpart() and get_maxextra().
     */
    Xapian::termcount get_wdf_upper_bound() const {
	return wdf_upper_bound_;
    }

    /// Total length of all documents in the collection.
    Xapian::totallength get_total_length() const {
	return total_length_;
    }

    /** A lower bound on the number of unique terms in any document in the
     *  shard.
     *
     *  This bound does not include any zero-length documents.
     *
     *  This should only be used by get_maxpart() and get_maxextra().
     *
     *  @since 1.5.0
     */
    Xapian::termcount get_unique_terms_upper_bound() const {
	return unique_terms_upper_bound_;
    }

    /** An upper bound on the number of unique terms in any document in the
     *  shard.
     *
     *  This should only be used by get_maxpart() and get_maxextra().
     *
     *  @since 1.5.0
     */
    Xapian::termcount get_unique_terms_lower_bound() const {
	return unique_terms_lower_bound_;
    }

    /** An upper bound on the maximum length of any document in the database.
     *
     *  @since 1.5.0
     */
    Xapian::termcount get_db_doclength_upper_bound() const {
	return db_doclength_upper_bound_;
    }

    /** A lower bound on the minimum length of any document in the database.
     *
     *  This bound does not include any zero-length documents.
     *
     *  @since 1.5.0
     */
    Xapian::termcount get_db_doclength_lower_bound() const {
	return db_doclength_lower_bound_;
    }

    /** A lower bound on the number of unique terms in any document in the
     *  database.
     *
     *  This bound does not include any zero-length documents.
     *
     *  @since 1.5.0
     */
    Xapian::termcount get_db_unique_terms_upper_bound() const {
	return db_unique_terms_upper_bound_;
    }

    /** An upper bound on the number of unique terms in any document in the
     *  database.
     *
     *  @since 1.5.0
     */
    Xapian::termcount get_db_unique_terms_lower_bound() const {
	return db_unique_terms_lower_bound_;
    }

    /** An upper bound on the wdf of this term in the database.
     *
     *  @since 1.5.0
     */
    Xapian::termcount get_db_wdf_upper_bound() const {
	return db_wdf_upper_bound_;
    }
};

/** Class implementing a "boolean" weighting scheme.
 *
 *  This weighting scheme gives all documents zero weight.
 */
class XAPIAN_VISIBILITY_DEFAULT BoolWeight : public Weight {
    BoolWeight * clone() const;

    void init(double factor);

  public:
    /** Construct a BoolWeight. */
    BoolWeight() {
	need_stat(IS_BOOLWEIGHT_);
    }

    std::string name() const;

    std::string serialise() const;
    BoolWeight * unserialise(const std::string & serialised) const;

    double get_sumpart(Xapian::termcount wdf,
		       Xapian::termcount doclen,
		       Xapian::termcount uniqterms,
		       Xapian::termcount wdfdocmax) const;
    double get_maxpart() const;

    BoolWeight * create_from_parameters(const char * params) const;
};

/// Xapian::Weight subclass implementing the tf-idf weighting scheme.
class XAPIAN_VISIBILITY_DEFAULT TfIdfWeight : public Weight {
  public:
    /** Wdf normalizations.
     *
     *  @since 1.5.0
     */
    enum class wdf_norm : unsigned char {
	/** None
	 *
	 *  wdfn=wdf
	 */
	NONE = 1,

	/** Boolean
	 *
	 *  wdfn=1 if term in document else wdfn=0
	 */
	BOOLEAN = 2,

	/** Square
	 *
	 *  wdfn=wdf*wdf
	 */
	SQUARE = 3,

	/** Logarithmic
	 *
	 *  wdfn=1+log<sub>e</sub>(wdf)
	 */
	LOG = 4,

	/** Pivoted
	 *
	 *  wdfn=(1+log(1+log(wdf)))*
	 *	 (1/(1-slope+(slope*doclen/avg_len)))+delta
	 */
	PIVOTED = 5,

	/** Log average
	 *
	 *  wdfn=(1+log(wdf))/
	 *	 (1+log(doclen/unique_terms))
	 */
	LOG_AVERAGE = 6,

	/** Augmented Log
	 *
	 *  wdfn=0.2+0.8*log(wdf+1)
	 */
	AUG_LOG = 7,

	/** Square Root
	 *
	 *  wdfn=sqrt(wdf-0.5)+1 if(wdf>0), else wdfn=0
	 */
	SQRT = 8,

	/** Augmented average term frequency
	 *
	 *  wdfn=0.9+0.1*(wdf/(doclen/unique_terms)) if(wdf>0), else wdfn=0
	 */
	AUG_AVERAGE = 9,

	/** Max wdf
	 *
	 *  wdfn=wdf/wdfdocmax
	 */
	MAX = 10,

	/** Augmented max wdf
	 *
	 *  wdfn=0.5+0.5*wdf/wdfdocmax if(wdf>0), else wdfn=0
	 */
	AUG = 11
    };

    /** Idf normalizations. */
    enum class idf_norm : unsigned char {
	/** None
	 *
	 *  idfn=1
	 */
	NONE = 1,

	/** TfIdf
	 *
	 *  idfn=log(N/Termfreq) where N is the number of documents
	 *  in collection and Termfreq is the number of documents which are
	 *  indexed by the term t.
	 */
	TFIDF = 2,

	/** Square
	 *
	 *  idfn=(log(N/Termfreq))²
	 */
	SQUARE = 3,

	/** Frequency
	 *
	 *  idfn=1/Termfreq
	 */
	FREQ = 4,

	/** Probability
	 *
	 *  idfn=log((N-Termfreq)/Termfreq)
	 */
	PROB = 5,

	/** Pivoted
	 *
	 *  idfn=log((N+1)/Termfreq)
	 */
	PIVOTED = 6,

	/** Global frequency IDF
	 *
	 *  idfn=Collfreq/Termfreq
	 */
	GLOBAL_FREQ = 7,

	/** Log global frequency IDF
	 *
	 *  idfn=log(Collfreq/Termfreq+1)
	 */
	LOG_GLOBAL_FREQ = 8,

	/** Incremented global frequency IDF
	 *
	 *  idfn=Collfreq/Termfreq+1
	 */
	INCREMENTED_GLOBAL_FREQ = 9,

	/** Square root global frequency IDF
	 *
	 *  idfn=sqrt(Collfreq/Termfreq-0.9)
	 */
	SQRT_GLOBAL_FREQ = 10
    };

    /** Weight normalizations. */
    enum class wt_norm : unsigned char {
	/** None
	 *
	 *  wtn=tfn*idfn
	 */
	NONE = 1
    };
  private:
    /// The parameter for normalization for the wdf.
    wdf_norm wdf_norm_;
    /// The parameter for normalization for the idf.
    idf_norm idf_norm_;
    /// The parameter for normalization for the document weight.
    wt_norm wt_norm_;

    /// The factor to multiply with the weight.
    double wqf_factor;

    /// Normalised IDF value (document-independent).
    double idfn;

    /// Parameters slope and delta in the Piv+ normalization weighting formula.
    double param_slope, param_delta;

    TfIdfWeight * clone() const;

    void init(double factor);

    /* When additional normalizations are implemented in the future, the additional statistics for them
       should be accessed by these functions. */
    double get_wdfn(Xapian::termcount wdf,
		    Xapian::termcount len,
		    Xapian::termcount uniqterms,
		    Xapian::termcount wdfdocmax,
		    wdf_norm wdf_normalization) const;
    double get_idfn(idf_norm idf_normalization) const;
    double get_wtn(double wt, wt_norm wt_normalization) const;

  public:
    /** Construct a TfIdfWeight
     *
     *  @param normalizations	A three character string indicating the
     *				normalizations to be used for the tf(wdf), idf
     *				and document weight.  (default: "ntn")
     *
     * The @a normalizations string works like so:
     *
     * @li The first character specifies the normalization for the wdf.  The
     *     following normalizations are currently supported:
     *
     *     @li 'n': None.      wdfn=wdf
     *     @li 'b': Boolean    wdfn=1 if term in document else wdfn=0
     *     @li 's': Square     wdfn=wdf*wdf
     *     @li 'l': Logarithmic wdfn=1+log<sub>e</sub>(wdf)
     *     @li 'P': Pivoted     wdfn=(1+log(1+log(wdf)))*(1/(1-slope+(slope*doclen/avg_len)))+delta
     *     @li 'L': Log average wdfn=(1+log(wdf))/(1+log(doclen/unique_terms))
     *     @li 'm': Max-wdf	wdfn=wdf/wdfdocmax
     *     @li 'a': Augmented max-wdf  wdfn=0.5+0.5*wdf/wdfdocmax
     *
     * @li The second character indicates the normalization for the idf.  The
     *     following normalizations are currently supported:
     *
     *     @li 'n': None    idfn=1
     *     @li 't': TfIdf   idfn=log(N/Termfreq) where N is the number of
     *         documents in collection and Termfreq is the number of documents
     *         which are indexed by the term t.
     *     @li 'p': Prob    idfn=log((N-Termfreq)/Termfreq)
     *     @li 'f': Freq    idfn=1/Termfreq
     *     @li 's': Squared idfn=(log(N/Termfreq))²
     *     @li 'P': Pivoted idfn=log((N+1)/Termfreq)
     *
     * @li The third and the final character indicates the normalization for
     *     the document weight.  The following normalizations are currently
     *     supported:
     *
     *     @li 'n': None wtn=tfn*idfn
     *
     * Implementing support for more normalizations of each type would require
     * extending the backend to track more statistics.
     */
    explicit TfIdfWeight(const std::string& normalizations)
	: TfIdfWeight(normalizations, 0.2, 1.0) {}

    /** Construct a TfIdfWeight
     *
     *  @param normalizations	A three character string indicating the
     *				normalizations to be used for the tf(wdf), idf
     *				and document weight.  (default: "ntn")
     *	@param slope		Extra parameter for "Pivoted" tf normalization.  (default: 0.2)
     *	@param delta		Extra parameter for "Pivoted" tf normalization.  (default: 1.0)
     *
     * The @a normalizations string works like so:
     *
     * @li The first character specifies the normalization for the wdf.  The
     *     following normalizations are currently supported:
     *
     *     @li 'n': None.      wdfn=wdf
     *     @li 'b': Boolean    wdfn=1 if term in document else wdfn=0
     *     @li 's': Square     wdfn=wdf*wdf
     *     @li 'l': Logarithmic wdfn=1+log<sub>e</sub>(wdf)
     *     @li 'P': Pivoted     wdfn=(1+log(1+log(wdf)))*(1/(1-slope+(slope*doclen/avg_len)))+delta
     *     @li 'm': Max-wdf	wdfn=wdf/wdfdocmax
     *     @li 'a': Augmented max-wdf  wdfn=0.5+0.5*wdf/wdfdocmax
     *
     * @li The second character indicates the normalization for the idf.  The
     *     following normalizations are currently supported:
     *
     *     @li 'n': None    idfn=1
     *     @li 't': TfIdf   idfn=log(N/Termfreq) where N is the number of
     *         documents in collection and Termfreq is the number of documents
     *         which are indexed by the term t.
     *     @li 'p': Prob    idfn=log((N-Termfreq)/Termfreq)
     *     @li 'f': Freq    idfn=1/Termfreq
     *     @li 's': Squared idfn=(log(N/Termfreq))²
     *     @li 'P': Pivoted idfn=log((N+1)/Termfreq)
     *
     * @li The third and the final character indicates the normalization for
     *     the document weight.  The following normalizations are currently
     *     supported:
     *
     *     @li 'n': None wtn=tfn*idfn
     *
     * Implementing support for more normalizations of each type would require
     * extending the backend to track more statistics.
     */
    TfIdfWeight(const std::string &normalizations, double slope, double delta);

    /** Construct a TfIdfWeight
     *
     *	@param wdf_norm_	The normalization for the wdf.
     *	@param idf_norm_	The normalization for the idf.
     *	@param wt_norm_		The normalization for the document weight.
     *
     * Implementing support for more normalizations of each type would require
     * extending the backend to track more statistics.
     */
    TfIdfWeight(wdf_norm wdf_normalization,
		idf_norm idf_normalization,
		wt_norm wt_normalization)
	: TfIdfWeight(wdf_normalization, idf_normalization,
		      wt_normalization, 0.2, 1.0) {}

    /** Construct a TfIdfWeight
     *
     *	@param wdf_norm_	The normalization for the wdf.
     *	@param idf_norm_	The normalization for the idf.
     *	@param wt_norm_		The normalization for the document weight.
     *	@param slope		Extra parameter for "Pivoted" tf normalization.
     *				(default: 0.2)
     *	@param delta		Extra parameter for "Pivoted" tf normalization.
     *				(default: 1.0)
     *
     * Implementing support for more normalizations of each type would require
     * extending the backend to track more statistics.
     */
    TfIdfWeight(wdf_norm wdf_norm_, idf_norm idf_norm_,
		wt_norm wt_norm_, double slope, double delta);

    /** Construct a TfIdfWeight using the default normalizations ("ntn"). */
    TfIdfWeight()
	: wdf_norm_(wdf_norm::NONE), idf_norm_(idf_norm::TFIDF),
	  wt_norm_(wt_norm::NONE), param_slope(0.2), param_delta(1.0)
    {
	need_stat(WQF);
	need_stat(TERMFREQ);
	need_stat(WDF);
	need_stat(WDF_MAX);
	need_stat(COLLECTION_SIZE);
    }

    std::string name() const;

    std::string serialise() const;
    TfIdfWeight * unserialise(const std::string & serialised) const;

    double get_sumpart(Xapian::termcount wdf,
		       Xapian::termcount doclen,
		       Xapian::termcount uniqterm,
		       Xapian::termcount wdfdocmax) const;
    double get_maxpart() const;

    TfIdfWeight * create_from_parameters(const char * params) const;
};


/// Xapian::Weight subclass implementing the BM25 probabilistic formula.
class XAPIAN_VISIBILITY_DEFAULT BM25Weight : public Weight {
    /// Factor to multiply the document length by.
    mutable Xapian::doclength len_factor;

    /// Factor combining all the document independent factors.
    mutable double termweight;

    /// The BM25 parameters.
    double param_k1, param_k2, param_k3, param_b;

    /// The minimum normalised document length value.
    Xapian::doclength param_min_normlen;

    BM25Weight * clone() const;

    void init(double factor);

  public:
    /** Construct a BM25Weight.
     *
     *  @param k1  A non-negative parameter controlling how influential
     *		   within-document-frequency (wdf) is.  k1=0 means that
     *		   wdf doesn't affect the weights.  The larger k1 is, the more
     *		   wdf influences the weights.  (default 1)
     *
     *  @param k2  A non-negative parameter which controls the strength of a
     *		   correction factor which depends upon query length and
     *		   normalised document length.  k2=0 disable this factor; larger
     *		   k2 makes it stronger.  (default 0)
     *
     *  @param k3  A non-negative parameter controlling how influential
     *		   within-query-frequency (wqf) is.  k3=0 means that wqf
     *		   doesn't affect the weights.  The larger k3 is, the more
     *		   wqf influences the weights.  (default 1)
     *
     *  @param b   A parameter between 0 and 1, controlling how strong the
     *		   document length normalisation of wdf is.  0 means no
     *		   normalisation; 1 means full normalisation.  (default 0.5)
     *
     *  @param min_normlen  A parameter specifying a minimum value for
     *		   normalised document length.  Normalised document length
     *		   values less than this will be clamped to this value, helping
     *		   to prevent very short documents getting large weights.
     *		   (default 0.5)
     */
    BM25Weight(double k1, double k2, double k3, double b, double min_normlen)
	: param_k1(k1), param_k2(k2), param_k3(k3), param_b(b),
	  param_min_normlen(min_normlen)
    {
	if (param_k1 < 0) param_k1 = 0;
	if (param_k2 < 0) param_k2 = 0;
	if (param_k3 < 0) param_k3 = 0;
	if (param_b < 0) {
	    param_b = 0;
	} else if (param_b > 1) {
	    param_b = 1;
	}
	need_stat(COLLECTION_SIZE);
	need_stat(RSET_SIZE);
	need_stat(TERMFREQ);
	need_stat(RELTERMFREQ);
	need_stat(WDF);
	need_stat(WDF_MAX);
	if (param_k2 != 0 || (param_k1 != 0 && param_b != 0)) {
	    need_stat(DOC_LENGTH_MIN);
	    need_stat(AVERAGE_LENGTH);
	}
	if (param_k1 != 0 && param_b != 0) need_stat(DOC_LENGTH);
	if (param_k2 != 0) {
	    need_stat(DOC_LENGTH);
	    need_stat(QUERY_LENGTH);
	}
	if (param_k3 != 0) need_stat(WQF);
    }

    BM25Weight()
	: param_k1(1), param_k2(0), param_k3(1), param_b(0.5),
	  param_min_normlen(0.5)
    {
	need_stat(COLLECTION_SIZE);
	need_stat(RSET_SIZE);
	need_stat(TERMFREQ);
	need_stat(RELTERMFREQ);
	need_stat(WDF);
	need_stat(WDF_MAX);
	need_stat(DOC_LENGTH_MIN);
	need_stat(AVERAGE_LENGTH);
	need_stat(DOC_LENGTH);
	need_stat(WQF);
    }

    std::string name() const;

    std::string serialise() const;
    BM25Weight * unserialise(const std::string & serialised) const;

    double get_sumpart(Xapian::termcount wdf,
		       Xapian::termcount doclen,
		       Xapian::termcount uniqterm,
		       Xapian::termcount wdfdocmax) const;
    double get_maxpart() const;

    double get_sumextra(Xapian::termcount doclen,
			Xapian::termcount uniqterms,
			Xapian::termcount wdfdocmax) const;
    double get_maxextra() const;

    BM25Weight * create_from_parameters(const char * params) const;
};

/// Xapian::Weight subclass implementing the BM25+ probabilistic formula.
class XAPIAN_VISIBILITY_DEFAULT BM25PlusWeight : public Weight {
    /// Factor to multiply the document length by.
    mutable Xapian::doclength len_factor;

    /// Factor combining all the document independent factors.
    mutable double termweight;

    /// The BM25+ parameters.
    double param_k1, param_k2, param_k3, param_b;

    /// The minimum normalised document length value.
    Xapian::doclength param_min_normlen;

    /// Additional parameter delta in the BM25+ formula.
    double param_delta;

    BM25PlusWeight * clone() const;

    void init(double factor);

  public:
    /** Construct a BM25PlusWeight.
     *
     *  @param k1  A non-negative parameter controlling how influential
     *		   within-document-frequency (wdf) is.  k1=0 means that
     *		   wdf doesn't affect the weights.  The larger k1 is, the more
     *		   wdf influences the weights.  (default 1)
     *
     *  @param k2  A non-negative parameter which controls the strength of a
     *		   correction factor which depends upon query length and
     *		   normalised document length.  k2=0 disable this factor; larger
     *		   k2 makes it stronger.  The paper which describes BM25+
     *		   ignores BM25's document-independent component (so implicitly
     *		   k2=0), but we support non-zero k2 too.  (default 0)
     *
     *  @param k3  A non-negative parameter controlling how influential
     *		   within-query-frequency (wqf) is.  k3=0 means that wqf
     *		   doesn't affect the weights.  The larger k3 is, the more
     *		   wqf influences the weights.  (default 1)
     *
     *  @param b   A parameter between 0 and 1, controlling how strong the
     *		   document length normalisation of wdf is.  0 means no
     *		   normalisation; 1 means full normalisation.  (default 0.5)
     *
     *  @param min_normlen  A parameter specifying a minimum value for
     *		   normalised document length.  Normalised document length
     *		   values less than this will be clamped to this value, helping
     *		   to prevent very short documents getting large weights.
     *		   (default 0.5)
     *
     *  @param delta  A parameter for pseudo tf value to control the scale
     *		      of the tf lower bound. Delta(δ) can be tuned for example
     *		      from 0.0 to 1.5 but BM25+ can still work effectively
     *		      across collections with a fixed δ = 1.0. (default 1.0)
     */
    BM25PlusWeight(double k1, double k2, double k3, double b,
		   double min_normlen, double delta)
	: param_k1(k1), param_k2(k2), param_k3(k3), param_b(b),
	  param_min_normlen(min_normlen), param_delta(delta)
    {
	if (param_k1 < 0) param_k1 = 0;
	if (param_k2 < 0) param_k2 = 0;
	if (param_k3 < 0) param_k3 = 0;
	if (param_delta < 0) param_delta = 0;
	if (param_b < 0) {
	    param_b = 0;
	} else if (param_b > 1) {
	    param_b = 1;
	}
	need_stat(COLLECTION_SIZE);
	need_stat(RSET_SIZE);
	need_stat(TERMFREQ);
	need_stat(RELTERMFREQ);
	need_stat(WDF);
	need_stat(WDF_MAX);
	if (param_k2 != 0 || (param_k1 != 0 && param_b != 0)) {
	    need_stat(DOC_LENGTH_MIN);
	    need_stat(AVERAGE_LENGTH);
	}
	if (param_k1 != 0 && param_b != 0) need_stat(DOC_LENGTH);
	if (param_k2 != 0) {
	    need_stat(DOC_LENGTH);
	    need_stat(QUERY_LENGTH);
	}
	if (param_k3 != 0) need_stat(WQF);
    }

    BM25PlusWeight()
	: param_k1(1), param_k2(0), param_k3(1), param_b(0.5),
	  param_min_normlen(0.5), param_delta(1)
    {
	need_stat(COLLECTION_SIZE);
	need_stat(RSET_SIZE);
	need_stat(TERMFREQ);
	need_stat(RELTERMFREQ);
	need_stat(WDF);
	need_stat(WDF_MAX);
	need_stat(DOC_LENGTH_MIN);
	need_stat(AVERAGE_LENGTH);
	need_stat(DOC_LENGTH);
	need_stat(WQF);
    }

    std::string name() const;

    std::string serialise() const;
    BM25PlusWeight * unserialise(const std::string & serialised) const;

    double get_sumpart(Xapian::termcount wdf,
		       Xapian::termcount doclen,
		       Xapian::termcount uniqterms,
		       Xapian::termcount wdfdocmax) const;
    double get_maxpart() const;

    double get_sumextra(Xapian::termcount doclen,
			Xapian::termcount uniqterms,
			Xapian::termcount wdfdocmax) const;
    double get_maxextra() const;

    BM25PlusWeight * create_from_parameters(const char * params) const;
};

/** Xapian::Weight subclass implementing the traditional probabilistic formula.
 *
 * This class implements the "traditional" Probabilistic Weighting scheme, as
 * described by the early papers on Probabilistic Retrieval.  BM25 generally
 * gives better results.
 *
 * TradWeight(k) is equivalent to BM25Weight(k, 0, 0, 1, 0), and since Xapian
 * 1.5.0 TradWeight is actually implemented as a subclass of BM25Weight.  In
 * earlier versions is was a separate class which was equivalent except it
 * returned weights (k+1) times smaller.
 *
 * @deprecated Use BM25Weight(k, 0, 0, 1, 0) instead.
 */
class XAPIAN_DEPRECATED_CLASS TradWeight : public BM25Weight
{
  public:
    /** Construct a TradWeight.
     *
     *  @param k  A non-negative parameter controlling how influential
     *		  within-document-frequency (wdf) and document length are.
     *		  k=0 means that wdf and document length don't affect the
     *		  weights.  The larger k is, the more they do.  (default 1)
     */
    explicit TradWeight(double k = 1.0) : BM25Weight(k, 0.0, 0.0, 1.0, 0.0) { }
};

/** This class implements the InL2 weighting scheme.
 *
 *  InL2 is a representative scheme of the Divergence from Randomness Framework
 *  by Gianni Amati.
 *
 *  This weighting scheme is useful for tasks that require early precision.
 *
 *  It uses the Inverse document frequency model (In), the Laplace method to
 *  find the aftereffect of sampling (L) and the second wdf normalization
 *  proposed by Amati to normalize the wdf in the document to the length of the
 *  document (H2).
 *
 *  For more information about the DFR Framework and the InL2 scheme, please
 *  refer to: Gianni Amati and Cornelis Joost Van Rijsbergen Probabilistic
 *  models of information retrieval based on measuring the divergence from
 *  randomness ACM Transactions on Information Systems (TOIS) 20, (4), 2002,
 *  pp. 357-389.
 */
class XAPIAN_VISIBILITY_DEFAULT InL2Weight : public Weight {
    /// The wdf normalization parameter in the formula.
    double param_c;

    /// The upper bound on the weight a term can give to a document.
    double upper_bound;

    /// The constant values which are used on every call to get_sumpart().
    double wqf_product_idf;
    double c_product_avlen;

    InL2Weight * clone() const;

    void init(double factor);

  public:
    /** Construct an InL2Weight.
     *
     *  @param c  A strictly positive parameter controlling the extent
     *		  of the normalization of the wdf to the document length. The
     *		  default value of 1 is suitable for longer queries but it may
     *		  need to be changed for shorter queries. For more information,
     *		  please refer to Gianni Amati's PHD thesis.
     */
    explicit InL2Weight(double c);

    InL2Weight()
    : param_c(1.0)
    {
	need_stat(AVERAGE_LENGTH);
	need_stat(DOC_LENGTH);
	need_stat(DOC_LENGTH_MIN);
	need_stat(DOC_LENGTH_MAX);
	need_stat(COLLECTION_SIZE);
	need_stat(WDF);
	need_stat(WDF_MAX);
	need_stat(WQF);
	need_stat(TERMFREQ);
    }

    std::string name() const;

    std::string serialise() const;
    InL2Weight * unserialise(const std::string & serialised) const;

    double get_sumpart(Xapian::termcount wdf,
		       Xapian::termcount doclen,
		       Xapian::termcount uniqterms,
		       Xapian::termcount wdfdocmax) const;
    double get_maxpart() const;

    InL2Weight * create_from_parameters(const char * params) const;
};

/** This class implements the IfB2 weighting scheme.
 *
 *  IfB2 is a representative scheme of the Divergence from Randomness Framework
 *  by Gianni Amati.
 *
 *  It uses the Inverse term frequency model (If), the Bernoulli method to find
 *  the aftereffect of sampling (B) and the second wdf normalization proposed
 *  by Amati to normalize the wdf in the document to the length of the document
 *  (H2).
 *
 *  For more information about the DFR Framework and the IfB2 scheme, please
 *  refer to: Gianni Amati and Cornelis Joost Van Rijsbergen Probabilistic
 *  models of information retrieval based on measuring the divergence from
 *  randomness ACM Transactions on Information Systems (TOIS) 20, (4), 2002,
 *  pp. 357-389.
 */
class XAPIAN_VISIBILITY_DEFAULT IfB2Weight : public Weight {
    /// The wdf normalization parameter in the formula.
    double param_c;

    /// The upper bound on the weight.
    double upper_bound;

    /// The constant values which are used for calculations in get_sumpart().
    double wqf_product_idf;
    double c_product_avlen;
    double B_constant;

    IfB2Weight * clone() const;

    void init(double factor);

  public:
    /** Construct an IfB2Weight.
     *
     *  @param c  A strictly positive parameter controlling the extent
     *		  of the normalization of the wdf to the document length. The
     *		  default value of 1 is suitable for longer queries but it may
     *		  need to be changed for shorter queries. For more information,
     *		  please refer to Gianni Amati's PHD thesis titled
     *		  Probabilistic Models for Information Retrieval based on
     *		  Divergence from Randomness.
     */
    explicit IfB2Weight(double c);

    IfB2Weight() : param_c(1.0) {
	need_stat(AVERAGE_LENGTH);
	need_stat(DOC_LENGTH);
	need_stat(DOC_LENGTH_MIN);
	need_stat(DOC_LENGTH_MAX);
	need_stat(COLLECTION_SIZE);
	need_stat(COLLECTION_FREQ);
	need_stat(WDF);
	need_stat(WDF_MAX);
	need_stat(WQF);
	need_stat(TERMFREQ);
    }

    std::string name() const;

    std::string serialise() const;
    IfB2Weight * unserialise(const std::string & serialised) const;

    double get_sumpart(Xapian::termcount wdf,
		       Xapian::termcount doclen,
		       Xapian::termcount uniqterm,
		       Xapian::termcount wdfdocmax) const;
    double get_maxpart() const;

    IfB2Weight * create_from_parameters(const char * params) const;
};

/** This class implements the IneB2 weighting scheme.
 *
 *  IneB2 is a representative scheme of the Divergence from Randomness
 *  Framework by Gianni Amati.
 *
 *  It uses the Inverse expected document frequency model (Ine), the Bernoulli
 *  method to find the aftereffect of sampling (B) and the second wdf
 *  normalization proposed by Amati to normalize the wdf in the document to the
 *  length of the document (H2).
 *
 *  For more information about the DFR Framework and the IneB2 scheme, please
 *  refer to: Gianni Amati and Cornelis Joost Van Rijsbergen Probabilistic
 *  models of information retrieval based on measuring the divergence from
 *  randomness ACM Transactions on Information Systems (TOIS) 20, (4), 2002,
 *  pp. 357-389.
 */
class XAPIAN_VISIBILITY_DEFAULT IneB2Weight : public Weight {
    /// The wdf normalization parameter in the formula.
    double param_c;

    /// The upper bound of the weight.
    double upper_bound;

    /// Constant values used in get_sumpart().
    double wqf_product_idf;
    double c_product_avlen;
    double B_constant;

    IneB2Weight * clone() const;

    void init(double factor);

  public:
    /** Construct an IneB2Weight.
     *
     *  @param c  A strictly positive parameter controlling the extent
     *		  of the normalization of the wdf to the document length. The
     *		  default value of 1 is suitable for longer queries but it may
     *		  need to be changed for shorter queries. For more information,
     *		  please refer to Gianni Amati's PHD thesis.
     */
    explicit IneB2Weight(double c);

    IneB2Weight() : param_c(1.0) {
	need_stat(AVERAGE_LENGTH);
	need_stat(DOC_LENGTH);
	need_stat(DOC_LENGTH_MIN);
	need_stat(DOC_LENGTH_MAX);
	need_stat(COLLECTION_SIZE);
	need_stat(WDF);
	need_stat(WDF_MAX);
	need_stat(WQF);
	need_stat(COLLECTION_FREQ);
	need_stat(TERMFREQ);
    }

    std::string name() const;

    std::string serialise() const;
    IneB2Weight * unserialise(const std::string & serialised) const;

    double get_sumpart(Xapian::termcount wdf,
		       Xapian::termcount doclen,
		       Xapian::termcount uniqterms,
		       Xapian::termcount wdfdocmax) const;
    double get_maxpart() const;

    IneB2Weight * create_from_parameters(const char * params) const;
};

/** This class implements the BB2 weighting scheme.
 *
 *  BB2 is a representative scheme of the Divergence from Randomness Framework
 *  by Gianni Amati.
 *
 *  It uses the Bose-Einstein probabilistic distribution (B) along with
 *  Stirling's power approximation, the Bernoulli method to find the
 *  aftereffect of sampling (B) and the second wdf normalization proposed by
 *  Amati to normalize the wdf in the document to the length of the document
 *  (H2).
 *
 *  For more information about the DFR Framework and the BB2 scheme, please
 *  refer to : Gianni Amati and Cornelis Joost Van Rijsbergen Probabilistic
 *  models of information retrieval based on measuring the divergence from
 *  randomness ACM Transactions on Information Systems (TOIS) 20, (4), 2002,
 *  pp. 357-389.
 */
class XAPIAN_VISIBILITY_DEFAULT BB2Weight : public Weight {
    /// The wdf normalization parameter in the formula.
    double param_c;

    /// The upper bound on the weight.
    double upper_bound;

    /// The constant values to be used in get_sumpart().
    double c_product_avlen;
    double B_constant;
    double wt;
    double stirling_constant_1;
    double stirling_constant_2;

    BB2Weight * clone() const;

    void init(double factor);

  public:
    /** Construct a BB2Weight.
     *
     *  @param c  A strictly positive parameter controlling the extent
     *		  of the normalization of the wdf to the document length. A
     *		  default value of 1 is suitable for longer queries but it may
     *		  need to be changed for shorter queries. For more information,
     *		  please refer to Gianni Amati's PHD thesis titled
     *		  Probabilistic Models for Information Retrieval based on
     *		  Divergence from Randomness.
     */
    explicit BB2Weight(double c);

    BB2Weight() : param_c(1.0) {
	need_stat(AVERAGE_LENGTH);
	need_stat(DOC_LENGTH);
	need_stat(DOC_LENGTH_MIN);
	need_stat(DOC_LENGTH_MAX);
	need_stat(COLLECTION_SIZE);
	need_stat(COLLECTION_FREQ);
	need_stat(WDF);
	need_stat(WDF_MAX);
	need_stat(WQF);
	need_stat(TERMFREQ);
    }

    std::string name() const;

    std::string serialise() const;
    BB2Weight * unserialise(const std::string & serialised) const;

    double get_sumpart(Xapian::termcount wdf,
		       Xapian::termcount doclen,
		       Xapian::termcount uniqterms,
		       Xapian::termcount wdfdocmax) const;
    double get_maxpart() const;

    BB2Weight * create_from_parameters(const char * params) const;
};

/** This class implements the DLH weighting scheme, which is a representative
 *  scheme of the Divergence from Randomness Framework by Gianni Amati.
 *
 *  This is a parameter free weighting scheme and it should be used with query
 *  expansion to obtain better results. It uses the HyperGeometric Probabilistic
 *  model and Laplace's normalization to calculate the risk gain.
 *
 *  For more information about the DFR Framework and the DLH scheme, please
 *  refer to :
 *  a.) Gianni Amati and Cornelis Joost Van Rijsbergen Probabilistic
 *  models of information retrieval based on measuring the divergence from
 *  randomness ACM Transactions on Information Systems (TOIS) 20, (4), 2002, pp.
 *  357-389.
 *  b.) FUB, IASI-CNR and University of Tor Vergata at TREC 2007 Blog Track.
 *  G. Amati and E. Ambrosi and M. Bianchi and C. Gaibisso and G. Gambosi.
 *  Proceedings of the 16th Text REtrieval Conference (TREC-2007), 2008.
 */
class XAPIAN_VISIBILITY_DEFAULT DLHWeight : public Weight {
    /// The upper bound on the weight.
    double upper_bound;

    /// The constant value to be used in get_sumpart().
    double log_constant;
    double wqf_product_factor;

    DLHWeight * clone() const;

    void init(double factor);

  public:
    DLHWeight() {
	need_stat(DOC_LENGTH);
	need_stat(COLLECTION_FREQ);
	need_stat(WDF);
	need_stat(WQF);
	need_stat(WDF_MAX);
	need_stat(DOC_LENGTH_MIN);
	need_stat(DOC_LENGTH_MAX);
	need_stat(TOTAL_LENGTH);
    }

    std::string name() const;

    std::string serialise() const;
    DLHWeight * unserialise(const std::string & serialised) const;

    double get_sumpart(Xapian::termcount wdf,
		       Xapian::termcount doclen,
		       Xapian::termcount uniqterms,
		       Xapian::termcount wdfdocmax) const;
    double get_maxpart() const;

    DLHWeight * create_from_parameters(const char * params) const;
};

/** This class implements the PL2 weighting scheme.
 *
 *  PL2 is a representative scheme of the Divergence from Randomness Framework
 *  by Gianni Amati.
 *
 *  This weighting scheme is useful for tasks that require early precision.
 *
 *  It uses the Poisson approximation of the Binomial Probabilistic distribution
 *  (P) along with Stirling's approximation for the factorial value, the Laplace
 *  method to find the aftereffect of sampling (L) and the second wdf
 *  normalization proposed by Amati to normalize the wdf in the document to the
 *  length of the document (H2).
 *
 *  For more information about the DFR Framework and the PL2 scheme, please
 *  refer to : Gianni Amati and Cornelis Joost Van Rijsbergen Probabilistic models
 *  of information retrieval based on measuring the divergence from randomness
 *  ACM Transactions on Information Systems (TOIS) 20, (4), 2002, pp. 357-389.
 */
class XAPIAN_VISIBILITY_DEFAULT PL2Weight : public Weight {
    /// The factor to multiply weights by.
    double factor;

    /// The wdf normalization parameter in the formula.
    double param_c;

    /// The upper bound on the weight.
    double upper_bound;

    /// Constants for a given term in a given query.
    double P1, P2;

    /// Set by init() to (param_c * get_average_length())
    double cl;

    PL2Weight * clone() const;

    void init(double factor_);

  public:
    /** Construct a PL2Weight.
     *
     *  @param c  A strictly positive parameter controlling the extent
     *		  of the normalization of the wdf to the document length. The
     *		  default value of 1 is suitable for longer queries but it may
     *		  need to be changed for shorter queries. For more information,
     *		  please refer to Gianni Amati's PHD thesis titled
     *		  Probabilistic Models for Information Retrieval based on
     *		  Divergence from Randomness.
     */
    explicit PL2Weight(double c);

    PL2Weight() : param_c(1.0) {
	need_stat(AVERAGE_LENGTH);
	need_stat(DOC_LENGTH);
	need_stat(DOC_LENGTH_MIN);
	need_stat(DOC_LENGTH_MAX);
	need_stat(COLLECTION_SIZE);
	need_stat(COLLECTION_FREQ);
	need_stat(WDF);
	need_stat(WDF_MAX);
	need_stat(WQF);
    }

    std::string name() const;

    std::string serialise() const;
    PL2Weight * unserialise(const std::string & serialised) const;

    double get_sumpart(Xapian::termcount wdf,
		       Xapian::termcount doclen,
		       Xapian::termcount uniqterms,
		       Xapian::termcount wdfdocmax) const;
    double get_maxpart() const;

    PL2Weight * create_from_parameters(const char * params) const;
};

/// Xapian::Weight subclass implementing the PL2+ probabilistic formula.
class XAPIAN_VISIBILITY_DEFAULT PL2PlusWeight : public Weight {
    /// The factor to multiply weights by.
    double factor;

    /// The wdf normalization parameter in the formula.
    double param_c;

    /// Additional parameter delta in the PL2+ weighting formula.
    double param_delta;

    /// The upper bound on the weight.
    double upper_bound;

    /// Constants for a given term in a given query.
    double P1, P2;

    /// Set by init() to (param_c * get_average_length())
    double cl;

    /// Set by init() to get_collection_freq()) / get_collection_size()
    double mean;

    /// Weight contribution of delta term in the PL2+ function
    double dw;

    PL2PlusWeight * clone() const;

    void init(double factor_);

  public:
    /** Construct a PL2PlusWeight.
     *
     *  @param c  A strictly positive parameter controlling the extent
     *		  of the normalization of the wdf to the document length. The
     *		  default value of 1 is suitable for longer queries but it may
     *		  need to be changed for shorter queries. For more information,
     *		  please refer to Gianni Amati's PHD thesis titled
     *		  Probabilistic Models for Information Retrieval based on
     *		  Divergence from Randomness.
     *
     *  @param delta  A parameter for pseudo tf value to control the scale
     *                of the tf lower bound. Delta(δ) should be a positive
     *                real number. It can be tuned for example from 0.1 to 1.5
     *                in increments of 0.1 or so. Experiments have shown that
     *                PL2+ works effectively across collections with a fixed δ = 0.8
     *                (default 0.8)
     */
    PL2PlusWeight(double c, double delta);

    PL2PlusWeight()
	: param_c(1.0), param_delta(0.8) {
	need_stat(AVERAGE_LENGTH);
	need_stat(DOC_LENGTH);
	need_stat(DOC_LENGTH_MIN);
	need_stat(DOC_LENGTH_MAX);
	need_stat(COLLECTION_SIZE);
	need_stat(COLLECTION_FREQ);
	need_stat(WDF);
	need_stat(WDF_MAX);
	need_stat(WQF);
    }

    std::string name() const;

    std::string serialise() const;
    PL2PlusWeight * unserialise(const std::string & serialised) const;

    double get_sumpart(Xapian::termcount wdf,
		       Xapian::termcount doclen,
		       Xapian::termcount uniqterms,
		       Xapian::termcount wdfdocmax) const;
    double get_maxpart() const;

    PL2PlusWeight * create_from_parameters(const char * params) const;
};

/** This class implements the DPH weighting scheme.
 *
 *  DPH is a representative scheme of the Divergence from Randomness Framework
 *  by Gianni Amati.
 *
 *  This is a parameter free weighting scheme and it should be used with query
 *  expansion to obtain better results. It uses the HyperGeometric Probabilistic
 *  model and Popper's normalization to calculate the risk gain.
 *
 *  For more information about the DFR Framework and the DPH scheme, please
 *  refer to :
 *  a.) Gianni Amati and Cornelis Joost Van Rijsbergen
 *  Probabilistic models of information retrieval based on measuring the
 *  divergence from randomness ACM Transactions on Information Systems (TOIS) 20,
 *  (4), 2002, pp. 357-389.
 *  b.) FUB, IASI-CNR and University of Tor Vergata at TREC 2007 Blog Track.
 *  G. Amati and E. Ambrosi and M. Bianchi and C. Gaibisso and G. Gambosi.
 *  Proceedings of the 16th Text Retrieval Conference (TREC-2007), 2008.
 */
class XAPIAN_VISIBILITY_DEFAULT DPHWeight : public Weight {
    /// The upper bound on the weight.
    double upper_bound;

    /// The constant value used in get_sumpart() .
    double log_constant;
    double wqf_product_factor;

    DPHWeight * clone() const;

    void init(double factor);

  public:
    /** Construct a DPHWeight. */
    DPHWeight() {
	need_stat(DOC_LENGTH);
	need_stat(COLLECTION_FREQ);
	need_stat(WDF);
	need_stat(WQF);
	need_stat(WDF_MAX);
	need_stat(DOC_LENGTH_MIN);
	need_stat(DOC_LENGTH_MAX);
	need_stat(TOTAL_LENGTH);
    }

    std::string name() const;

    std::string serialise() const;
    DPHWeight * unserialise(const std::string & serialised) const;

    double get_sumpart(Xapian::termcount wdf,
		       Xapian::termcount doclen,
		       Xapian::termcount uniqterms,
		       Xapian::termcount wdfdocmax) const;
    double get_maxpart() const;

    DPHWeight * create_from_parameters(const char * params) const;
};


/** Language Model weighting with Jelinek-Mercer smoothing.
 *
 * As described in:
 *
 * Zhai, C., & Lafferty, J.D. (2004). A study of smoothing methods for language
 * models applied to information retrieval. ACM Trans. Inf. Syst., 22, 179-214.
 *
 * @since 1.5.0
 */
class XAPIAN_VISIBILITY_DEFAULT LMJMWeight : public Weight {
    /// The factor to multiply weights by.
    double factor;

    /// Parameter controlling the smoothing.
    double param_lambda;

    /// Precalculated multiplier for use in weight calculations.
    double multiplier;

    LMJMWeight* clone() const;

    void init(double factor_);

  public:
    /** Construct a LMJMWeight.
     *
     *  @param lambda	A parameter strictly between 0 and 1 which linearly
     *			interpolates between the maximum likelihood model (the
     *			limit as λ→0) and the collection model (the limit as
     *			λ→1).
     *
     *			Values of λ around 0.1 are apparently optimal for short
     *			queries and around 0.7 for long queries.  If lambda is
     *			out of range (i.e. <= 0 or >= 1) then the λ value used
     *			is chosen dynamically based on the query length using
     *			the formula:
     *
     *			  (query_length - 1) / 10.0
     *
     *			The result is clamped to 0.1 for query_length <= 2, and
     *			to 0.7 for query_length >= 8.
     */
    explicit LMJMWeight(double lambda = 0.0) : param_lambda(lambda) {
	need_stat(WQF);
	need_stat(QUERY_LENGTH);
	need_stat(DOC_LENGTH);
	need_stat(WDF);
	need_stat(WDF_MAX);
	need_stat(COLLECTION_FREQ);
	need_stat(TOTAL_LENGTH);
	need_stat(DOC_LENGTH_MIN);
    }

    double get_sumpart(Xapian::termcount wdf,
		       Xapian::termcount doclen,
		       Xapian::termcount uniqterm,
		       Xapian::termcount wdfdocmax) const;

    double get_maxpart() const;

    std::string name() const;

    std::string serialise() const;
    LMJMWeight* unserialise(const std::string& serialised) const;

    LMJMWeight* create_from_parameters(const char* params) const;
};

/** Language Model weighting with Dirichlet or Dir+ smoothing.
 *
 * Dirichlet smoothing is as described in:
 *
 * Zhai, C., & Lafferty, J.D. (2004). A study of smoothing methods for language
 * models applied to information retrieval. ACM Trans. Inf. Syst., 22, 179-214.
 *
 * Dir+ is described in:
 *
 * Lv, Y., & Zhai, C. (2011). Lower-bounding term frequency normalization.
 * International Conference on Information and Knowledge Management.
 *
 * @since 1.5.0
 */
class XAPIAN_VISIBILITY_DEFAULT LMDirichletWeight : public Weight {
    /// The factor to multiply weights by.
    double factor;

    /// Parameter controlling the smoothing.
    double param_mu;

    /// A pseudo TF value to control the scale of the TF lower bound.
    double param_delta;

    /// Precalculated multiplier for use in weight calculations.
    double multiplier;

    /** Precalculated offset to add to every sumextra.
     *
     * This is needed because the formula can return a negative
     * term-independent weight.
     */
    double extra_offset;

    LMDirichletWeight* clone() const;

    void init(double factor_);

  public:
    /** Construct a LMDirichletWeight.
     *
     *  @param mu	A parameter which is > 0.  Default: 2000
     *  @param delta	A parameter which is >= 0, which is "a pseudo [wdf]
     *			value to control the scale of the [wdf] lower bound".
     *			If this parameter is > 0, then the smoothing is Dir+;
     *			if it's zero, it's Dirichlet.  Default: 0.05
     */
    explicit LMDirichletWeight(double mu = 2000.0, double delta = 0.05)
	: param_mu(mu), param_delta(delta) {
	need_stat(WQF);
	need_stat(QUERY_LENGTH);
	need_stat(DOC_LENGTH);
	need_stat(WDF);
	need_stat(WDF_MAX);
	need_stat(COLLECTION_FREQ);
	need_stat(TOTAL_LENGTH);
	need_stat(DOC_LENGTH_MIN);
	need_stat(DOC_LENGTH_MAX);
    }

    double get_sumpart(Xapian::termcount wdf,
		       Xapian::termcount doclen,
		       Xapian::termcount uniqterm,
		       Xapian::termcount wdfdocmax) const;

    double get_maxpart() const;

    double get_sumextra(Xapian::termcount doclen,
			Xapian::termcount,
			Xapian::termcount) const;

    double get_maxextra() const;

    std::string name() const;

    std::string serialise() const;
    LMDirichletWeight* unserialise(const std::string& serialised) const;

    LMDirichletWeight* create_from_parameters(const char* params) const;
};

/** Language Model weighting with Absolute Discount smoothing.
 *
 * As described in:
 *
 * Zhai, C., & Lafferty, J.D. (2004). A study of smoothing methods for language
 * models applied to information retrieval. ACM Trans. Inf. Syst., 22, 179-214.
 *
 * @since 1.5.0
 */
class XAPIAN_VISIBILITY_DEFAULT LMAbsDiscountWeight : public Weight {
    /// The factor to multiply weights by.
    double factor;

    /// Parameter controlling the smoothing.
    double param_delta;

    /// Precalculated multiplier for use in weight calculations.
    double multiplier;

    /** Precalculated offset to add to every sumextra.
     *
     * This is needed because the formula can return a negative
     * term-independent weight.
     */
    double extra_offset;

    LMAbsDiscountWeight* clone() const;

    void init(double factor_);

  public:
    /** Construct a LMAbsDiscountWeight.
     *
     *  @param delta	A parameter between 0 and 1.  Default: 0.7
     */
    explicit LMAbsDiscountWeight(double delta = 0.7) : param_delta(delta) {
	need_stat(WQF);
	need_stat(QUERY_LENGTH);
	need_stat(DOC_LENGTH);
	need_stat(WDF);
	need_stat(WDF_MAX);
	need_stat(COLLECTION_FREQ);
	need_stat(TOTAL_LENGTH);
	need_stat(DOC_LENGTH_MIN);
	need_stat(UNIQUE_TERMS);
	need_stat(DOC_LENGTH_MAX);
    }

    double get_sumpart(Xapian::termcount wdf,
		       Xapian::termcount,
		       Xapian::termcount uniqterm,
		       Xapian::termcount wdfdocmax) const;

    double get_maxpart() const;

    double get_sumextra(Xapian::termcount doclen,
			Xapian::termcount,
			Xapian::termcount) const;

    double get_maxextra() const;

    std::string name() const;

    std::string serialise() const;
    LMAbsDiscountWeight* unserialise(const std::string& serialised) const;

    LMAbsDiscountWeight* create_from_parameters(const char* params) const;
};

/** Language Model weighting with Two Stage smoothing.
 *
 * As described in:
 *
 * Zhai, C., & Lafferty, J.D. (2004). A study of smoothing methods for language
 * models applied to information retrieval. ACM Trans. Inf. Syst., 22, 179-214.
 *
 * @since 1.5.0
 */
class XAPIAN_VISIBILITY_DEFAULT LM2StageWeight : public Weight {
    /// The factor to multiply weights by.
    double factor;

    /// Parameter controlling the smoothing.
    double param_lambda;

    /// Parameter controlling the smoothing.
    double param_mu;

    /// Precalculated multiplier for use in weight calculations.
    double multiplier;

    /** Precalculated offset to add to every sumextra.
     *
     * This is needed because the formula can return a negative
     * term-independent weight.
     */
    double extra_offset;

    LM2StageWeight* clone() const;

    void init(double factor_);

  public:
    /** Construct a LM2StageWeight.
     *
     *  @param lambda	A parameter between 0 and 1 which linearly interpolates
     *			between the maximum likelihood model (at 0) and the
     *			collection model (at 1).  Default: 0.7
     *  @param mu	A parameter which is greater than 0.  Default: 2000
     */
    explicit LM2StageWeight(double lambda = 0.7, double mu = 2000.0)
	: param_lambda(lambda), param_mu(mu)
    {
	need_stat(WQF);
	need_stat(QUERY_LENGTH);
	need_stat(DOC_LENGTH);
	need_stat(WDF);
	need_stat(WDF_MAX);
	need_stat(COLLECTION_FREQ);
	need_stat(TOTAL_LENGTH);
	need_stat(DOC_LENGTH_MIN);
	need_stat(DOC_LENGTH_MAX);
    }

    double get_sumpart(Xapian::termcount wdf,
		       Xapian::termcount doclen,
		       Xapian::termcount uniqterm,
		       Xapian::termcount wdfdocmax) const;
    double get_maxpart() const;

    double get_sumextra(Xapian::termcount doclen,
			Xapian::termcount uniqterm,
			Xapian::termcount wdfdocmax) const;
    double get_maxextra() const;

    std::string name() const;

    std::string serialise() const;
    LM2StageWeight* unserialise(const std::string& serialised) const;

    LM2StageWeight* create_from_parameters(const char* params) const;
};

/** Xapian::Weight subclass implementing Coordinate Matching.
 *
 *  Each matching term scores one point.  See Managing Gigabytes, Second Edition
 *  p181.
 */
class XAPIAN_VISIBILITY_DEFAULT CoordWeight : public Weight {
    /// The factor to multiply weights by.
    double factor;

  public:
    CoordWeight * clone() const;

    void init(double factor_);

    /** Construct a CoordWeight. */
    CoordWeight() { }

    std::string name() const;

    std::string serialise() const;
    CoordWeight * unserialise(const std::string & serialised) const;

    double get_sumpart(Xapian::termcount wdf,
		       Xapian::termcount doclen,
		       Xapian::termcount uniqterms,
		       Xapian::termcount wdfdocmax) const;
    double get_maxpart() const;

    CoordWeight * create_from_parameters(const char * params) const;
};

/** Xapian::Weight subclass implementing Dice Coefficient.
 *
 *  Dice Coefficient measures the degree of similarity between
 *  pair of sets (ex. between two documents or a document and a query).
 *
 *  Jaccard coefficient and Cosine coefficient are other similarity
 *  coefficients.
 *
 * @since 1.5.0
 */
class XAPIAN_VISIBILITY_DEFAULT DiceCoeffWeight : public Weight {
    /// The numerator in the weight calculation.
    double numerator;

    /// Upper bound on the weight
    double upper_bound;

    void init(double factor);

  public:
    DiceCoeffWeight * clone() const;

    /** Construct a DiceCoeffWeight. */
    DiceCoeffWeight() {
	need_stat(WQF);
	need_stat(QUERY_LENGTH);
	need_stat(UNIQUE_TERMS);
	need_stat(UNIQUE_TERMS_MIN);
    }

    std::string name() const;

    std::string serialise() const;
    DiceCoeffWeight * unserialise(const std::string & serialised) const;

    double get_sumpart(Xapian::termcount wdf,
		       Xapian::termcount doclen,
		       Xapian::termcount uniqterm,
		       Xapian::termcount wdfdocmax) const;
    double get_maxpart() const;

    DiceCoeffWeight * create_from_parameters(const char * params) const;
};
}

#endif // XAPIAN_INCLUDED_WEIGHT_H
