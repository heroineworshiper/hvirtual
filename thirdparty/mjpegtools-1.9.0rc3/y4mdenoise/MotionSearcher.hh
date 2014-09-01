#ifndef __MOTION_SEARCHER_H__
#define __MOTION_SEARCHER_H__

// This file (C) 2004 Steven Boswell.  All rights reserved.
// Released to the public under the GNU General Public License.
// See the file COPYING for more information.

#include "config.h"
#include <assert.h>
#include "mjpeg_types.h"
#include "TemplateLib.hh"
#include "Limits.hh"
#include "DoublyLinkedList.hh"
#include "ReferenceFrame.hh"
#include "SetRegion2D.hh"
#include "BitmapRegion2D.hh"
#include "SearchBorder.hh"

// HACK: for development error messages.
#include <stdio.h>



// Define this to print region unions/subtractions.
#ifdef DEBUG_REGION2D
//	#define PRINTREGIONMATH
#endif // DEBUG_REGION2D



// Define this to print details of the search-border's progress.
#ifdef DEBUG_REGION2D
//	#define PRINT_SEARCHBORDER
#endif // DEBUG_REGION2D



// Define this to expand existing search-border regions, instead of
// using the pixel-sorter, when there are too many regions in the area
// already.
//#define EXPAND_REGIONS



// Define this to prevent reference-frame pixels from being used more
// than once.
#define USE_REFERENCEFRAMEPIXELS_ONCE



// Define this to throttle the output of the pixel-sorter, using only
// those matches with the lowest sum-of-absolute-differences.
#define THROTTLE_PIXELSORTER_WITH_SAD



// Define this to prune all regions in the area of any flood-filled
// region.
#define PRUNE_FLOODFILL_NEIGHBORS



// Define this to use bitmap regions to implement zero-motion
// flood-fill.
#define ZERO_MOTION_FLOOD_FILL_WITH_BITMAP_REGIONS



// Define this to use bitmap regions to implement match-throttle
// flood-fill.
//#define MATCH_THROTTLE_FLOOD_FILL_WITH_BITMAP_REGIONS



// Define this to use a bitmap region to implement the
// used-reference-pixel region.  Don't define it to use a set-based
// region to implement the used-reference-pixel region.
#define USED_REFERENCE_PIXELS_REGION_IS_BITMAP



// Define this to include the code for using the search-border.
//#define USE_SEARCH_BORDER



// We'll be using this variant of the search-window.
#ifdef EXPAND_REGIONS
	#define OPTIONALLY_SORT_PIXEL_GROUPS
#endif // EXPAND_REGIONS
#ifdef THROTTLE_PIXELSORTER_WITH_SAD
	#define CALCULATE_SAD
#endif // THROTTLE_PIXELSORTER_WITH_SAD
#include "SearchWindow.hh"
#undef CALCULATE_SAD
#undef OPTIONALLY_SORT_PIXEL_GROUPS



// The generic motion-searcher class.  It's parameterized by the size of
// elements in the pixels, the dimension of the pixels, the numeric
// type to use in tolerance calculations, the numeric type to use for
// pixel indices, a numeric type big enough to hold the product of the
// largest expected frame width/height, the width/height of pixel groups
// to operate on, a numeric type big enough to hold
// pixel-dimension * pixel-group-width * pixel-group-height bits and
// serve as an array index, and the types of pixels, reference pixels,
// and reference frames to operate on.
// When constructed, it's configured with the number of frames over
// which to accumulate pixel values, the search radius (in separate x
// and y directions), the error tolerances, and throttle values for the
// number of matches and the size of matches.
//
// Pixel values are tracked over several frames.  The idea is, if the
// motion searcher can prove that a particular pixel in several frames
// is really the same pixel, it can use all the pixel's values to
// calculate a more accurate value for it.  Therefore, output is
// delayed by the number of frames specified in the constructor, to
// give the motion-searcher the slack to do this.
//
// The motion-searcher works on groups of pixels.  It iterates
// through a frame, looking for groups of pixels within the search
// radius that match the current group (within the error tolerance).
// It sorts the found pixel-groups by sum-of-absolute-differences,
// and keeps the best match-count-throttle matches.  Once the search is
// done, it tries to flood-fill each match, and the first match to be
// large enough (i.e.  to be match-size-throttle pixel-groups in size or
// larger) is applied to the image.  Any areas of the frame not resolved
// by this method are new information, and new reference pixels are
// allocated for them.
template <class PIXEL_NUM, int DIM, class PIXEL_TOL, class PIXELINDEX,
	class FRAMESIZE, PIXELINDEX PGW, PIXELINDEX PGH,
	class SORTERBITMASK,
	class PIXEL = Pixel<PIXEL_NUM,DIM,PIXEL_TOL>,
	class REFERENCEPIXEL
		= ReferencePixel<PIXEL_TOL,PIXEL_NUM,DIM,PIXEL>,
	class REFERENCEFRAME
		= ReferenceFrame<REFERENCEPIXEL,PIXELINDEX,FRAMESIZE> >
class MotionSearcher
{
public:
	typedef PIXEL Pixel_t;
		// Our pixel type.

	typedef REFERENCEPIXEL ReferencePixel_t;
		// Our reference pixel type.

	typedef REFERENCEFRAME ReferenceFrame_t;
		// Our reference frame type.

	typedef PIXEL_NUM PixelValue_t;
		// The numeric type to use in pixel values, in each dimension
		// of our pixels.

	typedef PIXEL_TOL Tolerance_t;
		// The numeric type to use in tolerance calculations.

	MotionSearcher();
		// Default constructor.

	virtual ~MotionSearcher();
		// Destructor.

	void Init (Status_t &a_reStatus, int a_nFrames,
			PIXELINDEX a_tnWidth, PIXELINDEX a_tnHeight,
			PIXELINDEX a_tnSearchRadiusX, PIXELINDEX a_tnSearchRadiusY,
			PixelValue_t a_nZeroTolerance, PixelValue_t a_nTolerance,
			int a_nMatchCountThrottle, int a_nMatchSizeThrottle);
		// Initializer.  Provide the number of frames over which to
		// accumulate pixel data, the dimensions of the frames, the
		// search radius, the error tolerances, and the match throttles.

	const ReferenceFrame_t *GetFrameReadyForOutput (void);
		// If a frame is ready to be output, return it, otherwise return
		// NULL.
		// Call this once before each call to AddFrame(), to ensure that
		// AddFrame() has the space to accept another frame.  Note that
		// this implies the data in the returned frame will be
		// invalidated by AddFrame().

	void AddFrame (Status_t &a_reStatus, const Pixel_t *a_pPixels);
		// Add another frame to be analyzed into the system.
		// The digested version will eventually be returned by either
		// GetFrameReadyForOutput() or GetRemainingFrames().

	const ReferenceFrame_t *GetRemainingFrames (void);
		// Once there is no more input, call this repeatedly to get the
		// details of the remaining frames, until it returns NULL.
	
	void Purge (void);
		// Purge ourselves of temporary structures.
		// Should be called every once in a while (e.g. every 100
		// frames).

private:
	int m_nFrames;
		// The number of reference frames we use.

	PIXELINDEX m_tnWidth;
	PIXELINDEX m_tnHeight;
	FRAMESIZE m_tnPixels;
		// The dimensions of each reference frame.

	PIXELINDEX m_tnSearchRadiusX, m_tnSearchRadiusY;
		// The search radius, i.e. how far from the current pixel
		// group we look when searching for possible moved instances of
		// the group.

	Tolerance_t m_tnTolerance, m_tnTwiceTolerance;
		// The error tolerance, i.e. the largest difference we're
		// willing to tolerate between pixels before considering them
		// to be different.  Also, twice the tolerance.

	Tolerance_t m_tnZeroTolerance;
		// The error tolerance for the zero-motion pass.

	int m_nMatchCountThrottle;
		// How many matches we're willing to have for the current
		// pixel group before we decide the area is highly patterned
		// and use an alternative algorithm for detecting motion.
		// (The standard algorithm for detecting motion does a bad job
		// with highly patterned areas, using up way too much time and
		// space than would be reasonable.)

	int m_nMatchSizeThrottle;
		// The number of times the size of a pixel-group that the
		// biggest region in the area of the current pixel-group can
		// be before we just flood-fill it and apply it now.

	PixelAllocator<REFERENCEPIXEL,FRAMESIZE> m_oPixelPool;
		// Our source for new reference pixels.

	ReferenceFrame_t **m_ppFrames;
		// Our reference frames; an array of pointers to frames.

	int m_nFirstFrame, m_nLastFrame;
		// The range of frames that contain useful info.
		// When both equal 0, no frames contain useful info.
		// When m_nFirstFrame is 0 and m_nLastFrame is m_nFrames,
		// it's time for GetFrameReadyForOutput() to emit a frame.
		// When m_nFirstFrame is greater than zero but less than
		// m_nLastFrame, it means our client is calling
		// GetRemainingFrames().

	ReferenceFrame_t *m_pNewFrame;
		// The reference-frame representation of the new frame.

	ReferenceFrame_t *m_pReferenceFrame;
		// The reference frame, against which the new frame is
		// compared.
	
	const Pixel_t *m_pNewFramePixels;
		// The pixels of the new frame (i.e. the raw version).
	
	PIXELINDEX m_tnX, m_tnY;
		// The index of the current pixel group.  Actually the index
		// of the top-left pixel in the current pixel group.  This
		// gets moved in a zigzag pattern, back and forth across the
		// frame and then down, until the end of the frame is reached.
	
	PIXELINDEX m_tnStepX;
		// Whether we're zigging or zagging.

	typedef Region2D<PIXELINDEX,FRAMESIZE> BaseRegion_t;
		// The base class for all our region types.

	typedef SetRegion2D<PIXELINDEX,FRAMESIZE> Region_t;
	typedef typename Region_t::Allocator RegionAllocator_t;
		// How we use SetRegion2D<>.

	typedef BitmapRegion2D<PIXELINDEX,FRAMESIZE> BitmapRegion_t;
		// How we use BitmapRegion2D<>.

	typedef SearchBorder<PIXELINDEX,FRAMESIZE> SearchBorder_t;
		// The base class for the type of search-border we'll be using.
	
	typedef typename SearchBorder_t::MovedRegion MovedRegion;
		// A moved region of pixels that has been detected.
	
	RegionAllocator_t m_oRegionAllocator;
		// Used by all our set-regions to allocate their space.

	typedef Set<MovedRegion *,
		typename MovedRegion::SortBySizeThenMotionVectorLength>
		MovedRegionSet;
	MovedRegionSet m_setRegions;
		// All moving areas detected so far.
		// Sorted by decreasing size, then increasing motion vector
		// length, i.e. the order in which they should be applied to
		// the reference-frame version of the new frame.

#ifdef USED_REFERENCE_PIXELS_REGION_IS_BITMAP
	BitmapRegion_t m_oUsedReferencePixels;
#else // USED_REFERENCE_PIXELS_REGION_IS_BITMAP
	Region_t m_oUsedReferencePixels;
#endif // USED_REFERENCE_PIXELS_REGION_IS_BITMAP
		// The region describing all parts of the reference frame that
		// have been found in the new frame, in the zero-motion pass.

	MovedRegion m_oMatchThrottleRegion;
		// The region that's applied to the frame before motion
		// detection is done.  Allocated here to avoid lots of
		// creation & destruction.
	
	void ApplyRegionToNewFrame (Status_t &a_reStatus,
			const MovedRegion &a_rRegion);
		// Apply this region to the new frame.

	typedef SearchWindow<PIXEL_NUM,DIM,PIXEL_TOL,PIXELINDEX,FRAMESIZE,
			PGW,PGH,SORTERBITMASK,PIXEL,REFERENCEPIXEL,
			REFERENCEFRAME> SearchWindow_t;
		// The type of search-window we'll be using.
	
	SearchWindow_t m_oSearchWindow;
		// The search window.  It contains all the cells needed to
		// analyze the image.

#ifdef THROTTLE_PIXELSORTER_WITH_SAD

	// A pixel group that matches the current pixel-group, with the
	// given sum-of-absolute-differences.
	class MatchedPixelGroup
	{
	public:
		Tolerance_t m_tnSAD;
			// The sum-of-absolute-differences.
		const typename SearchWindow_t::PixelGroup *m_pGroup;
			// The pixel group.

		MatchedPixelGroup();
			// Default constructor.

		MatchedPixelGroup (Tolerance_t a_tnSAD,
				const typename SearchWindow_t::PixelGroup *a_pGroup);
			// Initializing constructor.

		~MatchedPixelGroup();
			// Destructor.

		// A comparison class, suitable for Set<>.
		class SortBySAD
		{
		public:
			inline bool operator() (const MatchedPixelGroup &a_rLeft,
				const MatchedPixelGroup &a_rRight) const;
		};
	};

	typedef Set<MatchedPixelGroup,
			typename MatchedPixelGroup::SortBySAD>
			MatchedPixelGroupSet;
		// The type for a set of matched pixel-groups.

	MatchedPixelGroupSet m_setMatches;
		// All the matches for the current pixel-group that we
		// want to use.

#endif // THROTTLE_PIXELSORTER_WITH_SAD

#ifdef USE_SEARCH_BORDER

	// The search-border, specialized to put completed regions into
	// m_setRegions.
	class SearchBorder : public SearchBorder_t
	{
	public:
		SearchBorder (MovedRegionSet &a_rsetRegions);
			// Constructor.  Provide a reference to the set where
			// completed regions will be stored.

		virtual void OnCompletedRegion (Status_t &a_reStatus,
				typename SearchBorder::MovedRegion *a_pRegion);
			// Tell SearchBorder_t how to hand us a completed region.
	
	private:
		MovedRegionSet &m_rsetRegions;
			// Our list of completed regions.
	};

	SearchBorder m_oSearchBorder;
		// The search border, i.e. all regions on the border between
		// the searched area and the not-yet-searched area, the regions
		// still under construction.

	FRAMESIZE SearchBorder_FloodFill (Status_t &a_reStatus,
			PIXELINDEX &a_rtnMotionX, PIXELINDEX &a_rtnMotionY);
		// Remove all regions that matched the current pixel-group,
		// except for the best one.  Expand it as far as it can go,
		// i.e. flood-fill in its area.  Then apply it to the new
		// frame now.  Returns the number of points flood-filled.

#endif // USE_SEARCH_BORDER

	// A class that helps implement the zero-motion flood-fill.
	class ZeroMotionFloodFillControl
#ifdef ZERO_MOTION_FLOOD_FILL_WITH_BITMAP_REGIONS
		: public BitmapRegion_t::FloodFillControl
#else // ZERO_MOTION_FLOOD_FILL_WITH_BITMAP_REGIONS
		: public Region_t::FloodFillControl
#endif // ZERO_MOTION_FLOOD_FILL_WITH_BITMAP_REGIONS
	{
	private:
#ifdef ZERO_MOTION_FLOOD_FILL_WITH_BITMAP_REGIONS
		typedef typename BitmapRegion_t::FloodFillControl BaseClass;
			// Keep track of who our base class is.
#else // ZERO_MOTION_FLOOD_FILL_WITH_BITMAP_REGIONS
		typedef typename Region_t::FloodFillControl BaseClass;
			// Keep track of who our base class is.
#endif // ZERO_MOTION_FLOOD_FILL_WITH_BITMAP_REGIONS

		MotionSearcher *m_pMotionSearcher;
			// The motion-searcher we're working for.

	public:
#ifdef ZERO_MOTION_FLOOD_FILL_WITH_BITMAP_REGIONS
		ZeroMotionFloodFillControl();
			// Default constructor.  Must be followed by Init().
#else // ZERO_MOTION_FLOOD_FILL_WITH_BITMAP_REGIONS
		ZeroMotionFloodFillControl
				(typename BaseClass::Allocator &a_rAllocator
					= Region_t::Extents::Imp::sm_oNodeAllocator);
			// Partially-initializing constructor.
			// Must be followed by Init().
#endif // ZERO_MOTION_FLOOD_FILL_WITH_BITMAP_REGIONS

		void Init (Status_t &a_reStatus,
				MotionSearcher *a_pMotionSearcher);
			// Initializer.

		// Redefined FloodFillControl methods.

		bool ShouldUseExtent (typename ZeroMotionFloodFillControl::BaseClass::Extent &a_rExtent);
			// Return true if the flood-fill should examine the given
			// extent.
		
		bool IsPointInRegion (PIXELINDEX a_tnX, PIXELINDEX a_tnY);
			// Returns true if the given point should be included in the
			// flood-fill.
	};

	friend class ZeroMotionFloodFillControl;
		// Allow the zero-motion flood-fill-control class direct access.
	
	ZeroMotionFloodFillControl m_oZeroMotionFloodFillControl;
		// Used to implement flood-filling the result of the
		// zero-motion search.
	
	// A class that helps implement the match-throttle flood-fill.
	class MatchThrottleFloodFillControl
#ifdef MATCH_THROTTLE_FLOOD_FILL_WITH_BITMAP_REGIONS
		: public BitmapRegion_t::FloodFillControl
#else // MATCH_THROTTLE_FLOOD_FILL_WITH_BITMAP_REGIONS
		: public Region_t::FloodFillControl
#endif // MATCH_THROTTLE_FLOOD_FILL_WITH_BITMAP_REGIONS
	{
	private:
#ifdef MATCH_THROTTLE_FLOOD_FILL_WITH_BITMAP_REGIONS
		typedef typename BitmapRegion_t::FloodFillControl BaseClass;
			// Keep track of who our base class is.
#else // MATCH_THROTTLE_FLOOD_FILL_WITH_BITMAP_REGIONS
		typedef typename Region_t::FloodFillControl BaseClass;
			// Keep track of who our base class is.
#endif // MATCH_THROTTLE_FLOOD_FILL_WITH_BITMAP_REGIONS
	public:
#ifdef MATCH_THROTTLE_FLOOD_FILL_WITH_BITMAP_REGIONS
		MatchThrottleFloodFillControl();
			// Default constructor.  Must be followed by Init().
#else // MATCH_THROTTLE_FLOOD_FILL_WITH_BITMAP_REGIONS
		MatchThrottleFloodFillControl
				(typename BaseClass::Allocator &a_rAllocator
					= Region_t::Extents::Imp::sm_oNodeAllocator);
			// Partially-initializing constructor.
			// Must be followed by Init().
#endif // MATCH_THROTTLE_FLOOD_FILL_WITH_BITMAP_REGIONS

		void Init (Status_t &a_reStatus,
				MotionSearcher *a_pMotionSearcher);
			// Initializer.

		void SetupForFloodFill (PIXELINDEX a_tnMotionX,
				PIXELINDEX a_tnMotionY);
			// Set up to to a flood-fill.  Provide the motion vector.
			// Call this before doing a flood-fill with this control
			// object.

		// Redefined FloodFillControl methods.

		bool ShouldUseExtent (typename Region_t::Extent &a_rExtent);
			// Return true if the flood-fill should examine the given
			// extent.
		
		bool IsPointInRegion (PIXELINDEX a_tnX, PIXELINDEX a_tnY);
			// Returns true if the given point should be included in the
			// flood-fill.

	private:
		MotionSearcher *m_pMotionSearcher;
			// The motion-searcher we're working for.

		PIXELINDEX m_tnMotionX, m_tnMotionY;
			// The motion vector to be used for this flood-fill.
	};

	friend class MatchThrottleFloodFillControl;
		// Allow the zero-motion flood-fill-control class direct access.
	
	MatchThrottleFloodFillControl m_oMatchThrottleFloodFillControl;
		// Used to implement flood-filling a region before
		// motion-searching is done.
};



// Default constructor.
template <class PIXEL_NUM, int DIM, class PIXEL_TOL, class PIXELINDEX,
	class FRAMESIZE, PIXELINDEX PGW, PIXELINDEX PGH,
	class SORTERBITMASK, class PIXEL, class REFERENCEPIXEL,
	class REFERENCEFRAME>
MotionSearcher<PIXEL_NUM,DIM,PIXEL_TOL,PIXELINDEX,FRAMESIZE,
		PGW,PGH,SORTERBITMASK,PIXEL,REFERENCEPIXEL,
		REFERENCEFRAME>::MotionSearcher()
	:
#ifdef USE_SEARCH_BORDER
	  m_oSearchBorder (m_setRegions),
#endif // USE_SEARCH_BORDER
									  m_oRegionAllocator (1048576),
	  m_oMatchThrottleRegion (m_oRegionAllocator)
#ifndef ZERO_MOTION_FLOOD_FILL_WITH_BITMAP_REGIONS
												 ,
	  m_oZeroMotionFloodFillControl (m_oRegionAllocator)
#endif // ZERO_MOTION_FLOOD_FILL_WITH_BITMAP_REGIONS
#ifndef MATCH_THROTTLE_FLOOD_FILL_WITH_BITMAP_REGIONS
													 	,
	  m_oMatchThrottleFloodFillControl (m_oRegionAllocator)
#endif // MATCH_THROTTLE_FLOOD_FILL_WITH_BITMAP_REGIONS
{
	// No frames yet.
	m_nFrames = 0;
	m_ppFrames = NULL;
	m_nFirstFrame = m_nLastFrame = 0;
	m_tnWidth = m_tnHeight = PIXELINDEX (0);
	m_tnPixels = FRAMESIZE (0);

	// No information on the sort of search to do yet.
	m_tnSearchRadiusX = m_tnSearchRadiusY = PIXELINDEX (0);
	m_tnZeroTolerance = m_tnTolerance = m_tnTwiceTolerance
		= PIXEL_TOL (0);
	m_nMatchCountThrottle = 0;
	m_nMatchSizeThrottle = 0;

	// No active search yet.
	m_tnX = m_tnY = m_tnStepX = PIXELINDEX (0);
	m_pNewFrame = NULL;
	m_pReferenceFrame = NULL;
	m_pNewFramePixels = NULL;
}



// Destructor.
template <class PIXEL_NUM, int DIM, class PIXEL_TOL, class PIXELINDEX,
	class FRAMESIZE, PIXELINDEX PGW, PIXELINDEX PGH,
	class SORTERBITMASK, class PIXEL, class REFERENCEPIXEL,
	class REFERENCEFRAME>
MotionSearcher<PIXEL_NUM,DIM,PIXEL_TOL,PIXELINDEX,FRAMESIZE,
	PGW,PGH,SORTERBITMASK,PIXEL,REFERENCEPIXEL,
	REFERENCEFRAME>::~MotionSearcher()
{
	// Free up any remaining moved regions.  (Testing for a non-zero
	// size is defined to be safe even if the set hasn't been
	// initialized, i.e. if we get destroyed before our Init() has been
	// called.)
	if (m_setRegions.Size() > 0)
	{
		typename MovedRegionSet::Iterator itHere;
			// The location of the next region to destroy.

		// Loop through the moved-regions set, remove each item,
		// destroy it.
		while (itHere = m_setRegions.Begin(),
			itHere != m_setRegions.End())
		{
			// Get the moved-region to destroy.
			MovedRegion *pRegion = *itHere;

			// Remove it from the set.
			m_setRegions.Erase (itHere);

			// Destroy the region.
			delete pRegion;
		}
	}

	// Destroy the reference frames.
	for (int i = 0; i < m_nFrames; i++)
	{
		m_ppFrames[i]->Reset();
		delete m_ppFrames[i];
	}
	delete[] m_ppFrames;
}



// Initializer.
template <class PIXEL_NUM, int DIM, class PIXEL_TOL, class PIXELINDEX,
	class FRAMESIZE, PIXELINDEX PGW, PIXELINDEX PGH,
	class SORTERBITMASK, class PIXEL, class REFERENCEPIXEL,
	class REFERENCEFRAME>
void
MotionSearcher<PIXEL_NUM,DIM,PIXEL_TOL,PIXELINDEX,FRAMESIZE,
	PGW,PGH,SORTERBITMASK,PIXEL,REFERENCEPIXEL,REFERENCEFRAME>::Init
	(Status_t &a_reStatus, int a_nFrames, PIXELINDEX a_tnWidth,
	PIXELINDEX a_tnHeight, PIXELINDEX a_tnSearchRadiusX,
	PIXELINDEX a_tnSearchRadiusY, PixelValue_t a_tnZeroTolerance,
	PixelValue_t a_tnTolerance, int a_nMatchCountThrottle,
	int a_nMatchSizeThrottle)
{
	int i;
		// Used to loop through things.
	FRAMESIZE tnPixels;
		// The number of pixels in each frame.

	// Make sure they didn't start us off with an error.
	assert (a_reStatus == g_kNoError);

	// Make sure they gave us a reasonable number of frames.
	assert (a_nFrames >= 2);

	// Make sure the width & height are reasonable.
	assert (a_tnWidth > PIXELINDEX (0));
	assert (a_tnHeight > PIXELINDEX (0));

	// Make sure the search radius is reasonable.
	assert (a_tnSearchRadiusX > PIXELINDEX (0)
		&& a_tnSearchRadiusY > PIXELINDEX (0)
		&& a_tnSearchRadiusX <= a_tnWidth
		&& a_tnSearchRadiusY <= a_tnHeight);

	// Make sure the match throttles are reasonable.
	assert (a_nMatchCountThrottle >= 0
		&& a_nMatchCountThrottle
			<= a_tnSearchRadiusX * a_tnSearchRadiusY);
	assert (a_nMatchSizeThrottle > 0);

	// Calculate the number of pixels in each frame.
	tnPixels = FRAMESIZE (a_tnWidth) * FRAMESIZE (a_tnHeight);

	// Initialize our pixel pool.
	m_oPixelPool.Initialize (a_reStatus, tnPixels
		* FRAMESIZE (a_nFrames));
	if (a_reStatus != g_kNoError)
		return;

	// Allocate space for our pointers to frames.
	m_ppFrames = new ReferenceFrame_t * [a_nFrames];
	if (m_ppFrames == NULL)
	{
		a_reStatus = g_kOutOfMemory;
		return;
	}
	// (Initialize each one to NULL, in case we run out of memory while
	// trying to allocate frames -- if we don't do this, we'll end up
	// trying to delete garbage pointers.)
	for (i = 0; i < a_nFrames; ++i)
		m_ppFrames[i] = NULL;
	// (Save this parameter now, to make it possible to destroy an
	// incompletely-initialized object.)
	m_nFrames = a_nFrames;

	// Allocate our reference frames.
	for (i = 0; i < a_nFrames; ++i)
	{
		// Allocate the next reference frame.
		m_ppFrames[i] = new ReferenceFrame_t (a_reStatus, a_tnWidth,
			a_tnHeight);
		if (m_ppFrames[i] == NULL)
		{
			a_reStatus = g_kOutOfMemory;
			return;
		}
		if (a_reStatus != g_kNoError)
			return;
	}

	// Initialize the search-window.
	m_oSearchWindow.Init (a_reStatus, a_tnWidth, a_tnHeight,
		a_tnSearchRadiusX, a_tnSearchRadiusY, a_tnTolerance);
	if (a_reStatus != g_kNoError)
		return;

#ifdef THROTTLE_PIXELSORTER_WITH_SAD

		// Initialize our set of matches.  (We'll use this to sort the
		// incoming matches by how closely it matches the current
		// pixel-group, and we'll throw away bad matches.)
		m_setMatches.Init (a_reStatus, true);
		if (a_reStatus != g_kNoError)
			return;

#endif // THROTTLE_PIXELSORTER_WITH_SAD

	// Initialize our moved-regions set.
	m_setRegions.Init (a_reStatus, true);
	if (a_reStatus != g_kNoError)
		return;

	// Initialize our used reference-pixels container.
#ifdef USED_REFERENCE_PIXELS_REGION_IS_BITMAP
	m_oUsedReferencePixels.Init (a_reStatus, a_tnWidth, a_tnHeight);
#else // USED_REFERENCE_PIXELS_REGION_IS_BITMAP
	m_oUsedReferencePixels.Init (a_reStatus, m_oRegionAllocator);
#endif // USED_REFERENCE_PIXELS_REGION_IS_BITMAP
	if (a_reStatus != g_kNoError)
		return;
#if defined (USED_REFERENCE_PIXELS_REGION_IS_BITMAP) && defined (DEBUG_REGION2D)
	// HACK: too expensive, test region class elsewhere.
	m_oUsedReferencePixels.SetDebug (false);
#endif // USED_REFERENCE_PIXELS_REGION_IS_BITMAP && DEBUG_REGION2D

	// Initialize our match-throttle region.
	m_oMatchThrottleRegion.Init (a_reStatus);
	if (a_reStatus != g_kNoError)
		return;

#ifdef USE_SEARCH_BORDER

	// Initialize the search-border.
	m_oSearchBorder.Init (a_reStatus, a_tnWidth, a_tnHeight, PGW, PGH);
	if (a_reStatus != g_kNoError)
		return;

#endif // USE_SEARCH_BORDER

	// Finally, store our parameters.  (Convert the tolerance value to
	// the format used internally.)
	//m_nFrames = a_nFrames;		(Stored above)
	m_tnWidth = a_tnWidth;
	m_tnHeight = a_tnHeight;
	m_tnPixels = tnPixels;
	m_tnSearchRadiusX = a_tnSearchRadiusX;
	m_tnSearchRadiusY = a_tnSearchRadiusY;
	m_tnZeroTolerance = Pixel_t::MakeTolerance (a_tnZeroTolerance);
	m_tnTolerance = Pixel_t::MakeTolerance (a_tnTolerance);
	m_tnTwiceTolerance = Pixel_t::MakeTolerance (2 * a_tnTolerance);
	m_nMatchCountThrottle = a_nMatchCountThrottle;
	m_nMatchSizeThrottle = a_nMatchSizeThrottle;

	// Initialize our flood-fill controllers.  (This happens after we
	// store our parameters, because these methods may need those
	// values.)
	m_oZeroMotionFloodFillControl.Init (a_reStatus, this);
	if (a_reStatus != g_kNoError)
		return;
	m_oMatchThrottleFloodFillControl.Init (a_reStatus, this);
	if (a_reStatus != g_kNoError)
		return;
}



// If a frame is ready to be output, return it, otherwise return
// NULL.
template <class PIXEL_NUM, int DIM, class PIXEL_TOL, class PIXELINDEX,
	class FRAMESIZE, PIXELINDEX PGW, PIXELINDEX PGH,
	class SORTERBITMASK, class PIXEL, class REFERENCEPIXEL,
	class REFERENCEFRAME>
const typename MotionSearcher<PIXEL_NUM,DIM,PIXEL_TOL,PIXELINDEX,
	FRAMESIZE, PGW,PGH,SORTERBITMASK,PIXEL,REFERENCEPIXEL,
	REFERENCEFRAME>::ReferenceFrame_t *
MotionSearcher<PIXEL_NUM,DIM,PIXEL_TOL,PIXELINDEX,FRAMESIZE,
	PGW,PGH,SORTERBITMASK,PIXEL,REFERENCEPIXEL,
	REFERENCEFRAME>::GetFrameReadyForOutput (void)
{
	ReferenceFrame_t *pFrame;
		// The frame to return to the caller.
	int i;
		// Used to loop through things.

	// If we have space for the new frame in our reference frames, then
	// it's not time to emit a frame yet.  (We delay emitting frames for
	// as long as possible, in order to calculate the most accurate
	// pixel values.)
	if (m_nFirstFrame != 0 || m_nLastFrame != m_nFrames)
		return NULL;

	// Get the frame to return to the caller.
	pFrame = m_ppFrames[0];

	// Shift the remaining frames down.  (This is technically bad for
	// performance, but we don't expect our callers to use enough
	// reference frames to make this a problem, and if they do, well,
	// I guess I'll fix the code.  Call this one of the very few
	// shortcuts taken in this project. :-)
	for (i = 1; i < m_nFrames; ++i)
		m_ppFrames[i - 1] = m_ppFrames[i];

	// Our caller will read the data in the frame.  By the time the
	// caller calls AddFrame(), we'll need to use this frame again.
	// So put it at the end of the list.
	--m_nLastFrame;
	m_ppFrames[m_nLastFrame] = pFrame;

	// Finally, return the frame to our caller.
	return pFrame;
}



// HACK: developer debugging output.
extern "C" { extern int frame, verbose; };

// Add another frame to be analyzed into the system.
template <class PIXEL_NUM, int DIM, class PIXEL_TOL, class PIXELINDEX,
	class FRAMESIZE, PIXELINDEX PGW, PIXELINDEX PGH,
	class SORTERBITMASK, class PIXEL, class REFERENCEPIXEL,
	class REFERENCEFRAME>
void
MotionSearcher<PIXEL_NUM,DIM,PIXEL_TOL,PIXELINDEX,FRAMESIZE,
	PGW,PGH,SORTERBITMASK,PIXEL,REFERENCEPIXEL,REFERENCEFRAME>::AddFrame
	(Status_t &a_reStatus, const Pixel_t *a_pPixels)
{
	FRAMESIZE i, x, y;
		// Used to loop through pixels.
	FRAMESIZE tnNotMovedPixels, tnMovedPixels, tnNotMovedFloodedPixels,
			tnFloodedPixels, tnNoMatchNewPixels, tnNewPixels;
		// Statistics on the result of our analysis -- the number of
		// pixels that didn't move, the number that moved, the number
		// found by flood-filling, and the number of new pixels.

	// Make sure they didn't start us off with an error.
	assert (a_reStatus == g_kNoError);

	// Make sure we can accept a new frame.
	assert (m_nFirstFrame == 0 && m_nLastFrame < m_nFrames);

	// Get the reference frame that will become the new frame.
	m_pNewFrame = m_ppFrames[m_nLastFrame];
	m_pNewFramePixels = a_pPixels;

	// Reset the new frame, so that it doesn't refer to any pixels.
	// (This frame was previously returned by GetFrameReadyForOutput(),
	// so we can't reset it until now.)
	m_pNewFrame->Reset();

	// Reset our statistics.
	tnNotMovedPixels = tnMovedPixels = tnNotMovedFloodedPixels
		= tnFloodedPixels = tnNoMatchNewPixels = tnNewPixels
		= FRAMESIZE (0);

	// If there is a previous frame, do motion-detection against it.
	if (m_nFirstFrame != m_nLastFrame)
	{
		PIXELINDEX tnLastX, tnLastY;
			// Used to zigzag through the frame.

		// Get the reference frame, i.e. the one that we'll do
		// motion-detection against.  (For now, that's the previous
		// frame.  Eventually, we'd like to do motion-detection against
		// several previous frames, but not yet.)
		m_pReferenceFrame = m_ppFrames[m_nLastFrame - 1];

		// Prepare to search within this frame.
		m_oSearchWindow.StartFrame (m_pReferenceFrame);

		// Start by processing parts of the image that aren't moving.
		// Loop through pixel-group-sized chunks of the image, find
		// pixel-groups within the specified tolerance, and set them up
		// in the new reference frame.  Then flood-fill that region,
		// to catch all the borders.
		m_oUsedReferencePixels.Clear();
#if 1
		// (Search for rows of at least 3 pixels.)
		for (i = y = 0; y < m_tnHeight; ++y)
		{
			typename Region_t::Extent oFoundExtent;
				// Any extent of matching pixels we found.
			bool bStartedExtent;
				// true if we've started finding an extent.

			oFoundExtent.m_tnY = y;
			bStartedExtent = false;
			for (x = 0; x <= m_tnWidth; ++x, ++i)
			{
				bool bPixelMatched;
					// True if the new-frame pixel matches the
					// corresponding reference-frame pixel.
	
				// Sanity check.
				assert (y * m_tnWidth + x == i);

				// If these pixels are within the tolerance, then use
				// the previous frame's value in the new frame.
				bPixelMatched = false;
				if (x < m_tnWidth)
				{
					ReferencePixel_t *pPrevPixel;
						// The pixel from the previous frame.

					// Get the two pixels to compare.
					pPrevPixel = m_pReferenceFrame->GetPixel (i);
					assert (pPrevPixel != NULL);
					const Pixel_t &rPrevPixel = pPrevPixel->GetValue();
					const Pixel_t &rNewPixel = a_pPixels[i];
	
					// Compare them.
					if (rPrevPixel.IsWithinTolerance (rNewPixel,
						m_tnZeroTolerance))
					{
						// Remember it matched.
						bPixelMatched = true;

#if 0
						// HACK
						fprintf (stderr, " (%d,%d)",
							int (i % m_tnWidth),
							int (i / m_tnWidth));
#endif
					}
				}

				// Build a region containing all the used
				// reference-frame pixels.
				if (bPixelMatched)
				{
					// This point is in the region.  Start a new
					// extent if we didn't have one already, and add
					// the point to it.
					if (!bStartedExtent)
					{
						oFoundExtent.m_tnXStart = x;
						bStartedExtent = true;
					}
					oFoundExtent.m_tnXEnd = x + 1;
				}

				// This point is not in the region.  Any extent
				// we're building is done.
				else if (bStartedExtent)
				{
					// Add this extent to the region, but only if it's
					// big enough.
					if (oFoundExtent.m_tnXEnd
						- oFoundExtent.m_tnXStart > 2)
					{
						ReferencePixel_t *pPrevPixel;
							// The pixel from the previous frame.
						PIXELINDEX tnX;
						FRAMESIZE tnI;
	
						for (tnX = oFoundExtent.m_tnXStart;
							 tnX < oFoundExtent.m_tnXEnd;
							 ++tnX)
						{
							// Calculate the pixel index.
							tnI = oFoundExtent.m_tnY * m_tnWidth + tnX;

							// Get the two pixels.
							pPrevPixel = m_pReferenceFrame
								->GetPixel (tnX, oFoundExtent.m_tnY);
							assert (pPrevPixel != NULL);
							const Pixel_t &rNewPixel = a_pPixels[tnI];
		
							// Accumulate the value from the new frame.
							pPrevPixel->AddSample (rNewPixel);
			
							// Store the pixel in the new reference
							// frame.
							m_pNewFrame->SetPixel (tnX,
								oFoundExtent.m_tnY, pPrevPixel);
						}
	
						m_oUsedReferencePixels.Merge (a_reStatus,
							oFoundExtent.m_tnY,
							oFoundExtent.m_tnXStart,
							oFoundExtent.m_tnXEnd);
						if (a_reStatus != g_kNoError)
							return;
			
						// That's more pixels that were found not to
						// have moved.
						tnNotMovedPixels += oFoundExtent.m_tnXEnd
							- oFoundExtent.m_tnXStart;
					}

					// Look for another extent.
					bStartedExtent = false;
				}
			}
			--i;	// (we slightly overshot above)

			// Make sure we finished any extent we started.
			assert (!bStartedExtent);
		}
#else
		// (Search for entire pixel-groups.)
		y = 0;
		for (;;)
		{
			PIXELINDEX tnPixelX, tnPixelY;
				// Used to loop through pixels in the pixel-group.

			x = 0;
			for (;;)
			{
				ReferencePixel_t *pPrevPixel;
					// The pixel from the previous frame.
	
				// Loop through the pixels to compare, see if they all
				// match within the tolerance.
				for (tnPixelY = y;
					 tnPixelY < y + PGH;
					 ++tnPixelY)
				{
					for (tnPixelX = x;
						 tnPixelX < x + PGW;
						 ++tnPixelX)
					{
						// Get the two pixels to compare.
						pPrevPixel = m_pReferenceFrame->GetPixel
							(tnPixelX, tnPixelY);
						assert (pPrevPixel != NULL);
						const Pixel_t &rPrevPixel
							= pPrevPixel->GetValue();
						const Pixel_t &rNewPixel
							= a_pPixels[tnPixelY * m_tnHeight
								+ tnPixelX];
		
						// Compare them.
						if (!rPrevPixel.IsWithinTolerance (rNewPixel,
							m_tnZeroTolerance))
						{
							// No match.
							goto noMatch;
						}
					}
				}

				// These pixels are within the tolerance.  Use the
				// previous frame's value in the new frame.
				for (tnPixelY = y;
					 tnPixelY < y + PGH;
					 ++tnPixelY)
				{
					for (tnPixelX = x;
						 tnPixelX < x + PGW;
						 ++tnPixelX)
					{
						// Get the two pixels.
						pPrevPixel = m_pReferenceFrame->GetPixel
							(tnPixelX, tnPixelY);
						assert (pPrevPixel != NULL);
						const Pixel_t &rNewPixel
							= a_pPixels[tnPixelY * m_tnHeight
								+ tnPixelX];
		
						// Accumulate the value from the new frame.
						pPrevPixel->AddSample (rNewPixel);
		
						// Store the pixel in the new reference
						// frame.
						m_pNewFrame->SetPixel (tnPixelX, tnPixelY,
							pPrevPixel);
					}

#ifdef USE_REFERENCEFRAMEPIXELS_ONCE

					// Add this extent to the region.
					m_oUsedReferencePixels.Union (a_reStatus, tnPixelY,
						x, x + PGW);
					if (a_reStatus != g_kNoError)
						return;

#endif // USE_REFERENCEFRAMEPIXELS_ONCE
				}

				// Remember how many not-moved pixels we found.
				tnNotMovedPixels += PGW * PGH;

noMatch:
				// Now move X forward, but in a way that handles
				// frames whose dimensions are not even multiples of
				// the pixel-group dimension.
				if (x + PGW == m_tnWidth)
					break;
				x += PGW;
				if (x > m_tnWidth - PGW)
					x = m_tnWidth - PGW;
			}

			// Now move Y forward, but in a way that handles
			// frames whose dimensions are not even multiples of
			// the pixel-group dimension.
			if (y + PGH == m_tnHeight)
				break;
			y += PGH;
			if (y > m_tnHeight - PGH)
				y = m_tnHeight - PGH;
		}
#endif

		// All zero-motion pixel-group-sized chunks have been found.
		// Now flood-fill the region, to smoothly resolve all the
		// borders.  (Presently, this will set all the relevant
		// new-frame pixels, which is probably a layering violation.)
		m_oUsedReferencePixels.FloodFill (a_reStatus,
			m_oZeroMotionFloodFillControl, false);
		if (a_reStatus != g_kNoError)
			return;

		// Remember how many not-moved pixels we found.
		tnNotMovedPixels = m_oUsedReferencePixels.NumberOfPoints();

		// Find all search-window cells that contain used reference
		// pixels, and invalidate them.
		m_oSearchWindow.Prune (m_oUsedReferencePixels, PIXELINDEX (0),
			PIXELINDEX (0));

		// Now do the motion-compensated denoising.  Start in the
		// upper-left corner of the frame, and zigzag down the frame
		// (i.e. move all the way right, then down one line, then all
		// the way left, then down one line, etc.).  Look for matches
		// for the current pixel-group in the reference frame, and
		// build regions of such matches.
		//
		// (Skip it if they turned motion-detection off.)
		// (Skip it if the zero-motion case resolved all pixels.)
		if (m_nMatchCountThrottle > 0
		&& tnNotMovedPixels != m_tnPixels)
		{
			m_tnX = m_tnY = 0;
			tnLastX = m_tnWidth - PGW;
			tnLastY = m_tnHeight - PGH;
			m_tnStepX = 1;
#ifdef USE_SEARCH_BORDER
			m_oSearchBorder.StartFrame (a_reStatus);
#endif // USE_SEARCH_BORDER
			if (a_reStatus != g_kNoError)
				return;
			for (;;)
			{
				typename SearchWindow_t::PixelGroup oCurrentGroup;
					// The current pixel-group.
				typename SearchWindow_t::PixelSorterIterator itMatch;
					// Used to search for matches for the current
					// pixel-group.
#ifdef THROTTLE_PIXELSORTER_WITH_SAD
				typename MatchedPixelGroupSet::ConstIterator
						itBestMatch;
					// One of the best matches found.
				Tolerance_t tnSAD;
					// The sum-of-absolute-differences between the
					// current pixel group and the match.
#endif // THROTTLE_PIXELSORTER_WITH_SAD
				const typename SearchWindow_t::PixelGroup *pMatch;
					// A pixel-group that matches the current pixel,
					// within the configured tolerance.
				FRAMESIZE tnMatches;
					// The number of matches found.
				FRAMESIZE tnLargestMatch, tnSmallestMatch;
					// The largest/smallest regions that matched this
					// pixel-group.
	
				// Create the current pixel-group.  If any of its pixels
				// have been resolved, skip it.
				{
					PIXELINDEX x, y;
						// Used to loop through the current
						// pixel-group's pixels.
	
					for (y = 0; y < PGH; ++y)
					{
						for (x = 0; x < PGW; ++x)
						{
							PIXELINDEX tnPixelX, tnPixelY;
								// The index of the current pixel.
		
							// Calculate the index of the current pixel.
							tnPixelX = m_tnX + x;
							tnPixelY = m_tnY + y;
		
							// If this pixel has been resolved already,
							// skip this pixel-group.
							if (m_pNewFrame->GetPixel (tnPixelX,
									tnPixelY) != NULL)
								goto nextGroup;
		
							// Set the pixel value in the pixel-group.
							oCurrentGroup.m_atPixels[y][x]
								= a_pPixels[FRAMESIZE (tnPixelY)
									* FRAMESIZE (m_tnWidth)
									+ FRAMESIZE (tnPixelX)];
						}
					}
				}
	
				// Tell the pixel-group where it is.
				oCurrentGroup.m_tnX = m_tnX;
				oCurrentGroup.m_tnY = m_tnY;
	
#if 0
				// HACK
				fprintf (stderr, "Now checking pixel-group, frame %d, "
					"x %d, y %d", frame, int (m_tnX), int (m_tnY));
#endif
	
				// We have two ways to find matches for the current
				// pixel-group.  If there are few existing matches in
				// the area, search for them in the pixel-sorter tree.
				// If there are enough existing matches, just try to
				// expand those.  When we're through, if there are too
				// many matches in the area, and the region sizes have
				// hit a certain limit, just pick the best one,
				// flood-fill it, and get rid of all the other regions
				// in the area.
				tnMatches = 0;
				tnLargestMatch = 0;
				tnSmallestMatch = Limits<FRAMESIZE>::Max;
	
#ifdef EXPAND_REGIONS
				// If it's OK to just expand the existing regions, do
				// so.  (Estimating that most regions have a
				// current-border presence as well as a last-border
				// presence, we do this when we have match-throttle
				// regions in the area.)
				if (m_setBorderRegions.Size()
					> int ((PGH + 1) * m_nMatchCountThrottle))
				{
					typename IntersectingRegionsSet::ConstIterator
							itHere, itNext;
						// Used to loop through the regions.
	
					// HACK
					//fprintf (stderr, ", expanding existing "
					//	"regions.\n");
	
					// Set up the search-window in a radius around the
					// current pixel-group.  (We don't need the pixel
					// sorter, so don't pay for it.)
					m_oSearchWindow.PrepareForSearch (a_reStatus,
						false);
					if (a_reStatus != g_kNoError)
						return;
		
					// Loop through the ranges of equal motion-vectors,
					// check just that pixel-group, and if it matches,
					// add it to the system.
					for (itHere = m_setBorderRegions.Begin();
						 itHere != m_setBorderRegions.End();
						 itHere = itNext)
					{
						// Find the next motion-vector.
						BorderExtentBoundary oKey = *itHere;
						oKey.m_bIsEnding = true;
						oKey.m_pRegion = NULL;
						itNext = m_setBorderRegions.LowerBound (oKey);
	
#ifndef NDEBUG
						// Sanity check: make sure we got exactly where
						// we expected to.
						assert (itNext == m_setBorderRegions.End()
						|| (*itNext).m_tnMotionX
							!= (*itHere).m_tnMotionX
						|| (*itNext).m_tnMotionY
							!= (*itHere).m_tnMotionY);
						--itNext;
						assert ((*itNext).m_tnMotionX
							== (*itHere).m_tnMotionX
						&& (*itNext).m_tnMotionY
							== (*itHere).m_tnMotionY);
						++itNext;
#endif // NDEBUG
						
						// Check just that one pixel-group.
						PIXELINDEX tnX
							= (m_tnX + (*itHere).m_tnMotionX);
						PIXELINDEX tnY
							= (m_tnY + (*itHere).m_tnMotionY);
						if (tnX < 0 || tnX >= m_tnWidth - PGW + 1
						|| tnY < 0 || tnY >= m_tnHeight - PGH + 1)
							continue;
						const SearchWindowCell &rCell
							= m_oSearchWindow.GetCell (tnY, tnX);
						if (rCell.m_pForward != NULL)
						{
							assert (rCell.m_tnX
								== m_tnX + (*itHere).m_tnMotionX);
							assert (rCell.m_tnY
								== m_tnY + (*itHere).m_tnMotionY);
	
							if (rCell.IsWithinTolerance (oCurrentGroup,
								m_tnTolerance
#ifdef THROTTLE_PIXELSORTER_WITH_SAD
											 , tnSAD
#endif // THROTTLE_PIXELSORTER_WITH_SAD
													))
							{
#ifdef PRINT_SEARCHBORDER
								if (frame == 61 && DIM == 2)
								fprintf (stderr, "Found match at "
									"(%d,%d), motion vector (%d,%d)\n",
									int (m_tnX), int (m_tnY),
									int (rCell.m_tnX - m_tnX),
									int (rCell.m_tnY - m_tnY));
#endif

#ifndef USE_SEARCH_BORDER
	#error Expand-regions code needs the search-border
#endif // USE_SEARCH_BORDER
								// Add this match to our growing set of
								// moved regions.
								FRAMESIZE tnThisMatch
									= m_oSearchBorder.AddNewMatch
										(a_reStatus, rCell);
								if (a_reStatus != g_kNoError)
									return;
	
								// That's one more match.
								++tnMatches;
	
								// Keep track of the smallest/largest
								// region.
								tnSmallestMatch = Min (tnSmallestMatch,
									tnThisMatch);
								tnLargestMatch = Max (tnLargestMatch,
									tnThisMatch);
							}
						}
					}
				}
#endif // EXPAND_REGIONS
	
				// If the expanding-regions code couldn't run, or if it
				// didn't find any matches, then use the pixel-sorter to
				// find matches for the current pixel-group.
				if (tnMatches == 0)
				{
					// HACK
					//fprintf (stderr, ", consulting pixel-sorter.\n");
	
#ifndef THROTTLE_PIXELSORTER_WITH_SAD
					PIXELINDEX tnBestMotionX, tnBestMotionY;
					FRAMESIZE tnBestMotionXXYY;
						// The best match found, and its length.
#endif // THROTTLE_PIXELSORTER_WITH_SAD

					// Set up the search-window in a radius around the
					// current pixel-group.
					m_oSearchWindow.PrepareForSearch (a_reStatus, true);
					if (a_reStatus != g_kNoError)
						return;
					
					// Search for matches for the current pixel-group
					// within the search radius.
					m_oSearchWindow.StartSearch (itMatch,
						oCurrentGroup);
#ifdef THROTTLE_PIXELSORTER_WITH_SAD
					m_setMatches.Clear();
					while (pMatch = m_oSearchWindow.FoundNextMatch
						(itMatch, tnSAD), pMatch != NULL)
#else // THROTTLE_PIXELSORTER_WITH_SAD
					tnBestMotionX = m_tnSearchRadiusX + 1;
					tnBestMotionY = m_tnSearchRadiusY + 1;
					tnBestMotionXXYY = FRAMESIZE (tnBestMotionX)
						* FRAMESIZE (tnBestMotionX)
						+ FRAMESIZE (tnBestMotionY)
						* FRAMESIZE (tnBestMotionY);
					while (pMatch = m_oSearchWindow.FoundNextMatch
						(itMatch), pMatch != NULL)
#endif // THROTTLE_PIXELSORTER_WITH_SAD
					{
#ifdef PRINT_SEARCHBORDER
						if (frame == 61 && DIM == 2)
						fprintf (stderr, "Found match at (%d,%d), "
							"motion vector (%d,%d)\n",
							int (m_tnX), int (m_tnY),
							int (pMatch->m_tnX - m_tnX),
							int (pMatch->m_tnY - m_tnY));
#endif // PRINT_SEARCHBORDER
		
						// Make sure this match is within the search
						// radius.
						assert (AbsoluteValue (pMatch->m_tnX - m_tnX)
							<= m_tnSearchRadiusX);
						assert (AbsoluteValue (pMatch->m_tnY - m_tnY)
							<= m_tnSearchRadiusY);
		
#ifdef THROTTLE_PIXELSORTER_WITH_SAD

						// If this match is better than our worst so
						// far, get rid of our worst & use the new one
						// instead.
						if (m_setMatches.Size()
							== m_nMatchCountThrottle)
						{
							typename MatchedPixelGroupSet::Iterator
									itWorst;
								// The worst match found so far.
		
							// Get the worst match.  (It's at the end of
							// the list.)
							itWorst = m_setMatches.End();
							--itWorst;
		
							// If the new item is better than our worst,
							// get rid of our worst to make room for the
							// new item.
							if (tnSAD < (*itWorst).m_tnSAD)
								m_setMatches.Erase (itWorst);
						}
		
						// If this match is close enough to the current
						// pixel-group, make a note of it.
						if (m_setMatches.Size() < m_nMatchCountThrottle)
						{
							m_setMatches.Insert (a_reStatus,
								MatchedPixelGroup (tnSAD, pMatch));
							if (a_reStatus != g_kNoError)
								return;
						}

#else // THROTTLE_PIXELSORTER_WITH_SAD

#ifdef USE_SEARCH_BORDER

						// Add this match to our growing set of moved
						// regions.
						FRAMESIZE tnThisMatch
							= m_oSearchBorder.AddNewMatch (a_reStatus,
								*pMatch);
						if (a_reStatus != g_kNoError)
							return;

						// Keep track of the smallest/largest region.
						tnSmallestMatch = Min (tnSmallestMatch,
							tnThisMatch);
						tnLargestMatch = Max (tnLargestMatch,
							tnThisMatch);

#else // USE_SEARCH_BORDER

						PIXELINDEX tnMotionX, tnMotionY;
						FRAMESIZE tnMotionXXYY;
							// The motion vector of this match.

						// Calculate the motion vector of this match.
						tnMotionX = pMatch->m_tnX - m_tnX;
						tnMotionY = pMatch->m_tnY - m_tnY;
						tnMotionXXYY = FRAMESIZE (tnMotionX)
							* FRAMESIZE (tnMotionX)
							+ FRAMESIZE (tnMotionY)
							* FRAMESIZE (tnMotionY);

						// If this is the best match so far (i.e. the
						// one closest to the origin), use it.  (This
						// isn't a great test, but it's the one that
						// was accidentally being used for a long time,
						// and the results seemed OK, so what the heck.)
						if (tnBestMotionXXYY > tnMotionXXYY)
						{
							tnBestMotionXXYY = tnMotionXXYY;
							tnBestMotionX = tnMotionX;
							tnBestMotionY = tnMotionY;
						}

#endif // USE_SEARCH_BORDER
	
						// That's one more match.
						++tnMatches;

#endif // THROTTLE_PIXELSORTER_WITH_SAD
					}
		
#ifdef THROTTLE_PIXELSORTER_WITH_SAD

					// Now loop through all the good matches found,
					// flood-fill each one, and use the first one that
					// fills a large enough area.
					for (itBestMatch = m_setMatches.Begin();
						 itBestMatch != m_setMatches.End();
						 ++itBestMatch)
					{
						PIXELINDEX tnMotionX, tnMotionY;
							// The motion vector of this match.

						// Get the current match.
						const MatchedPixelGroup &rMatch = *itBestMatch;

						// Calculate its motion vector.
						tnMotionX = rMatch.m_pGroup->m_tnX - m_tnX;
						tnMotionY = rMatch.m_pGroup->m_tnY - m_tnY;

#ifdef USE_SEARCH_BORDER
						// Add this match to our growing set of moved
						// regions.
						FRAMESIZE tnThisMatch
							= m_oSearchBorder.AddNewMatch (a_reStatus,
								tnMotionX, tnMotionY);
						if (a_reStatus != g_kNoError)
							return;
	
						// That's one more match.
						++tnMatches;
	
						// Keep track of the smallest/largest region.
						tnSmallestMatch = Min (tnSmallestMatch,
							tnThisMatch);
						tnLargestMatch = Max (tnLargestMatch,
							tnThisMatch);
#else // USE_SEARCH_BORDER
						// Set up a region describing the current
						// pixel-group.
						m_oMatchThrottleRegion.Clear();
						{
							PIXELINDEX tnY;
								// Used to loop through the
								// pixel-group's lines.

							for (tnY = m_tnY;
								 tnY < m_tnY + PGH;
								 ++tnY)
							{
								m_oMatchThrottleRegion.Merge
									(a_reStatus, tnY, m_tnX,
									m_tnX + PGW);
								if (a_reStatus != g_kNoError)
									return;
							}
						}
		
						// Set its motion vector.
						m_oMatchThrottleRegion.SetMotionVector
							(tnMotionX, tnMotionY);

						// Flood-fill this match, so as to get its full
						// extent.
						m_oMatchThrottleFloodFillControl
							.SetupForFloodFill (tnMotionX, tnMotionY);
						m_oMatchThrottleRegion.FloodFill (a_reStatus,
							m_oMatchThrottleFloodFillControl, false);
						if (a_reStatus != g_kNoError)
							return;

						// Get the size of the flood-filled region.
						FRAMESIZE tnThisMatch
							= m_oMatchThrottleRegion.NumberOfPoints();

						// If this match is big enough, keep it.
						if (tnThisMatch
							>= m_nMatchSizeThrottle * PGH * PGW)
						{
							ApplyRegionToNewFrame (a_reStatus,
								m_oMatchThrottleRegion);
							if (a_reStatus != g_kNoError)
								return;
		
							// That's one more match.
							++tnMatches;

							// That's more pixels found by
							// flood-filling.
							if (tnMotionX == 0 && tnMotionY == 0)
								tnNotMovedFloodedPixels
									+= tnThisMatch;
							else
								tnFloodedPixels += tnThisMatch;
		
							// Stop looking.
							break;
						}
#endif // USE_SEARCH_BORDER
					}

					// All done with the matches.
					m_setMatches.Clear();

#else // THROTTLE_PIXELSORTER_WITH_SAD

#ifndef USE_SEARCH_BORDER

					// Now flood-fill the best match and apply it to
					// the new frame.
					if (tnMatches > 0)
					{
						PIXELINDEX tnY;
							// Used to loop through the pixel-group
							// lines.
	
						// Set up the region with the current
						// pixel-group.
						m_oMatchThrottleRegion.Clear();
						for (tnY = m_tnY;
							 tnY < m_tnY + PGH;
							 ++tnY)
						{
							m_oMatchThrottleRegion.Union (a_reStatus,
								tnY, m_tnX, m_tnX + PGW);
							if (a_reStatus != g_kNoError)
								return;
						}
			
						// Set its motion vector.
						m_oMatchThrottleRegion.SetMotionVector
							(tnBestMotionX, tnBestMotionY);
	
						// Flood-fill this match, so as to get its full
						// extent.
						m_oMatchThrottleFloodFillControl
							.SetupForFloodFill (tnBestMotionX,
								tnBestMotionY);
						m_oMatchThrottleRegion.FloodFill (a_reStatus,
							m_oMatchThrottleFloodFillControl, false);
						if (a_reStatus != g_kNoError)
							return;
	
						// Get the size of the flood-filled region.
						FRAMESIZE tnThisMatch
							= m_oMatchThrottleRegion.NumberOfPoints();
	
						// Apply the match to the new frame right now.
						ApplyRegionToNewFrame (a_reStatus,
							m_oMatchThrottleRegion);
						if (a_reStatus != g_kNoError)
							return;
	
						// That's more pixels found by flood-filling.
						if (tnBestMotionX == 0 && tnBestMotionY == 0)
							tnNotMovedFloodedPixels += tnThisMatch;
						else
							tnFloodedPixels += tnThisMatch;
					}
#endif // !USE_SEARCH_BORDER
#endif // THROTTLE_PIXELSORTER_WITH_SAD
				}
	
				// If no matches were found for the current pixel-group,
				// and there are no active border-regions in the area,
				// then we've found new information.
				// NOTE: this dramatically speeds up the analysis of
				// frames that contain mostly new info, but probably
				// impacts quality.  Flood-fills are allowed to override
				// the result, though.
				if (tnMatches == 0
#ifdef USE_SEARCH_BORDER
					&& m_oSearchBorder.NumberOfActiveRegions() == 0
#endif // USE_SEARCH_BORDER
																   )
				{
					PIXELINDEX tnX, tnY;

					for (tnY = m_tnY; tnY < m_tnY + PGH; ++tnY)
					{
						for (tnX = m_tnX; tnX < m_tnX + PGW; ++tnX)
						{
							// Allocate a new reference pixel.
							ReferencePixel_t *pNewPixel
								= m_oPixelPool.Allocate();
				
							// Store the new pixel in the reference
							// frame.
							m_pNewFrame->SetPixel (tnX, tnY, pNewPixel);
				
							// Give it the value from the new frame.
							pNewPixel->AddSample (a_pPixels[tnY
								* m_tnWidth + tnX]);
						}
					}
				}

#ifdef USE_SEARCH_BORDER

				// If it's time to flood-fill, do so.
				if (tnMatches >= FRAMESIZE (m_nMatchCountThrottle)
					|| tnLargestMatch >= FRAMESIZE
						(m_nMatchSizeThrottle * PGH * PGW))
				{
					FRAMESIZE tnFlooded;
					PIXELINDEX tnMotionX, tnMotionY;
	
					// Take the biggest region that the current
					// pixel-group matched, flood-fill it, and apply it
					// to the new frame now, eliminating all competing
					// moved-regions.
					tnFlooded = SearchBorder_FloodFill (a_reStatus,
						tnMotionX, tnMotionY);
					if (a_reStatus != g_kNoError)
						return;
	
#if 0
					// HACK
					fprintf (stderr, "Match throttle: frame %d, "
						"x %03d, y %03d, %03d matches, "
						"%04d flood     \r", frame,
						int (m_tnX), int (m_tnY), int (tnMatches),
						int (tnFlooded));
#endif
	
					// That's more pixels found by flood-filling.
					if (tnMotionX == 0 && tnMotionY == 0)
						tnNotMovedFloodedPixels += tnFlooded;
					else
						tnFloodedPixels += tnFlooded;
				}

#endif // USE_SEARCH_BORDER

nextGroup:
				// Move to the next pixel-group.
				if (m_tnStepX == 1 && m_tnX == tnLastX
				|| m_tnStepX == -1 && m_tnX == 0)
				{
					// We need to move down a line.  If we're already on
					// the last line, we're done with the frame.
					if (m_tnY == tnLastY)
						break;
	
					// Move down a line.
					m_oSearchWindow.MoveDown();
#ifdef USE_SEARCH_BORDER
					m_oSearchBorder.MoveDown (a_reStatus);
#endif // USE_SEARCH_BORDER
					if (a_reStatus != g_kNoError)
						return;
					++m_tnY;
					
					// HACK
					//fprintf (stderr, "Now on line %d\r", int (m_tnY));
	
					// Now move across the frame in the other direction,
					// i.e. zigzag.
					m_tnStepX = -m_tnStepX;
				}
				else if (m_tnStepX == 1)
				{
					// Move right one pixel.
					m_oSearchWindow.MoveRight();
#ifdef USE_SEARCH_BORDER
					m_oSearchBorder.MoveRight (a_reStatus);
#endif // USE_SEARCH_BORDER
					if (a_reStatus != g_kNoError)
						return;
					++m_tnX;
				}
				else
				{
					// Move left one pixel.
					assert (m_tnStepX == -1);
					m_oSearchWindow.MoveLeft();
#ifdef USE_SEARCH_BORDER
					m_oSearchBorder.MoveLeft (a_reStatus);
#endif // USE_SEARCH_BORDER
					if (a_reStatus != g_kNoError)
						return;
					--m_tnX;
				}
			}
#ifdef USE_SEARCH_BORDER
			m_oSearchBorder.FinishFrame (a_reStatus);
#endif // USE_SEARCH_BORDER
			if (a_reStatus != g_kNoError)
				return;
	
			// We've found all the possible moved regions between the
			// new frame and the reference frame.
			// Loop through the moved regions found by motion-detection,
			// apply each one to the new frame.
			//fprintf (stderr, "Applied extents:");	// HACK
			while (m_setRegions.Size() > 0)
			{
				typename MovedRegionSet::Iterator itRegion;
				MovedRegion *pRegion;
					// The moved region to apply next.
				PIXELINDEX tnMotionX, tnMotionY;
					// The region's motion vector.
		
				// Get the moved region to apply next.
				itRegion = m_setRegions.Begin();
				pRegion = *itRegion;
				m_setRegions.Erase (itRegion);
				pRegion->GetMotionVector (tnMotionX, tnMotionY);
		
				// Flood-fill the candidate region, re-testing all the
				// extents.  This removes parts that have been resolved
				// already, plus it expands the region to its fullest
				// extent.
				m_oMatchThrottleFloodFillControl.SetupForFloodFill
					(tnMotionX, tnMotionY);
				pRegion->FloodFill (a_reStatus,
					m_oMatchThrottleFloodFillControl, true);
				if (a_reStatus != g_kNoError)
					return;
		
				// If that makes it smaller than the next highest-
				// priority region we found, put it back in & try again.
				itRegion = m_setRegions.Begin();
				if (itRegion != m_setRegions.End()
				&& pRegion->NumberOfPoints()
					< (*itRegion)->NumberOfPoints())
				{
					// Are there enough points left in this region for
					// us to bother with it?
					if (pRegion->NumberOfPoints() <= 2 * PGW * PGH)
					{
						// No.  Just get rid of it.
						delete pRegion;
					}
					else
					{
						// Yes.  Put the region back into our set, to be
						// tried later.
#ifndef NDEBUG
						typename MovedRegionSet::InsertResult
							oInsertResult =
#endif // NDEBUG
							m_setRegions.Insert (a_reStatus, pRegion);
						if (a_reStatus != g_kNoError)
						{
							delete pRegion;
							return;
						}
						assert (oInsertResult.m_bInserted);
					}
		
					// Try the region that's now the highest priority.
					continue;
				}
		
				// Apply this region to the new frame.
				ApplyRegionToNewFrame (a_reStatus, *pRegion);
				if (a_reStatus != g_kNoError)
				{
					delete pRegion;
					return;
				}
	
				// That's more moved pixels.
				tnMovedPixels += pRegion->NumberOfPoints();
	
				// We're done with this region.
				delete pRegion;
			}
		}

		// Prepare for searching the next frame.
		m_oSearchWindow.FinishFrame();
	}

	// Motion-searching is done.  Loop through the reference frame's
	// pixels, find any unresolved pixels, and create a new pixel for
	// them, using the data in the new frame.
	for (i = 0; i < m_tnPixels; ++i)
	{
		ReferencePixel_t *pNewPixel;

		// If this pixel is still unresolved, give it the value of
		// the corresponding pixel in the new frame.
		pNewPixel = m_pNewFrame->GetPixel (i);
		if (pNewPixel == NULL)
		{
			// Allocate a new reference pixel.
			ReferencePixel_t *pNewPixel = m_oPixelPool.Allocate();

			// Store the new pixel in the reference frame.
			m_pNewFrame->SetPixel (i, pNewPixel);

			// Give it the value from the new frame.
			pNewPixel->AddSample (a_pPixels[i]);

			// That's one more new pixel.
			++tnNewPixels;
		}
		else if (pNewPixel != NULL
			&& pNewPixel->GetFrameReferences() == 1)
		{
			// Count up the earlier-found new pixel.
			++tnNoMatchNewPixels;
		}
	}

	// Make sure all pixels were accounted for.
	assert (tnNotMovedPixels + tnMovedPixels + tnNotMovedFloodedPixels
		+ tnFloodedPixels + tnNoMatchNewPixels + tnNewPixels
		== m_tnPixels);

	// All done.  Remember that the data in the new frame is now valid.
	++m_nLastFrame;

#ifndef NDEBUG
	// Make sure none of the pixels have more references than we have
	// frames.  (Sanity check.)
	for (i = 0; i < m_tnPixels; ++i)
	{
		int16_t nRefs = m_pNewFrame->GetPixel (i)->GetFrameReferences();
		assert (nRefs > 0);
		assert (nRefs <= m_nFrames);
	}
#endif // NDEBUG

	// We'll have a new reference-frame and new-frame in the next pass.
	m_pReferenceFrame = m_pNewFrame = NULL;
	m_pNewFramePixels = NULL;

	// HACK: print the pixel statistics.
	if (verbose >= 1)
	fprintf (stderr, "Frame %d: %.1f%% not-moved, "
		//"%.1f%%+%.1f%% flood, %.1f%% moved, %.1f%%+%.1f%% new\n",
			"%.1f%%+%.1f%% moved, %.1f%%+%.1f%% new\n",
			frame,
			(float (tnNotMovedPixels) * 100.0f / float (m_tnPixels)),
			(float (tnNotMovedFloodedPixels) * 100.0f
				/ float (m_tnPixels)),
			(float (tnFloodedPixels) * 100.0f / float (m_tnPixels)),
			//(float (tnMovedPixels) * 100.0f / float (m_tnPixels)),
			(float (tnNoMatchNewPixels) * 100.0f / float (m_tnPixels)),
			(float (tnNewPixels) * 100.0f / float (m_tnPixels)));
	
#ifndef NDEBUG
	// Print the allocation totals.
	fprintf (stderr, "%lu regions, %lu pixel-sorters, "
#ifdef USE_SEARCH_BORDER
													 "%lu RUCs"
#endif // USE_SEARCH_BORDER
															 "\n",
		MovedRegion::GetInstances(),
		m_oSearchWindow.GetPixelSorterNodeCount()
#ifdef USE_SEARCH_BORDER
		, m_oSearchBorder.GetRegionUnderConstructionCount()
#endif // USE_SEARCH_BORDER
														   );
#endif // NDEBUG
}



// Once there is no more input, call this repeatedly to get the
// details of the remaining frames, until it returns NULL.
template <class PIXEL_NUM, int DIM, class PIXEL_TOL, class PIXELINDEX,
	class FRAMESIZE, PIXELINDEX PGW, PIXELINDEX PGH,
	class SORTERBITMASK, class PIXEL, class REFERENCEPIXEL,
	class REFERENCEFRAME>
const typename MotionSearcher<PIXEL_NUM,DIM,PIXEL_TOL,PIXELINDEX,
	FRAMESIZE,PGW,PGH,SORTERBITMASK,PIXEL,REFERENCEPIXEL,
	REFERENCEFRAME>::ReferenceFrame_t *
MotionSearcher<PIXEL_NUM,DIM,PIXEL_TOL,PIXELINDEX,FRAMESIZE,
	PGW,PGH,SORTERBITMASK,PIXEL,REFERENCEPIXEL,
	REFERENCEFRAME>::GetRemainingFrames (void)
{
	ReferenceFrame_t *pFrame;
		// The frame to return to the caller.

	// If there are no frames left, let our caller know.
	if (m_nFirstFrame == m_nLastFrame)
		return NULL;

	// Get the frame to return to the caller.
	pFrame = m_ppFrames[m_nFirstFrame];

	// Remember not to hand it to the caller ever again.
	++m_nFirstFrame;

	// Finally, return the frame to our caller.
	return pFrame;
}



// Purge ourselves of temporary structures.
template <class PIXEL_NUM, int DIM, class PIXEL_TOL, class PIXELINDEX,
	class FRAMESIZE, PIXELINDEX PGW, PIXELINDEX PGH,
	class SORTERBITMASK, class PIXEL, class REFERENCEPIXEL,
	class REFERENCEFRAME>
void
MotionSearcher<PIXEL_NUM,DIM,PIXEL_TOL,PIXELINDEX,FRAMESIZE,
	PGW,PGH,SORTERBITMASK,PIXEL,REFERENCEPIXEL,
	REFERENCEFRAME>::Purge (void)
{
	// Clear out the pixel-sorter.
	m_oSearchWindow.PurgePixelSorter();
}



#ifdef THROTTLE_PIXELSORTER_WITH_SAD

// Default constructor.
template <class PIXEL_NUM, int DIM, class PIXEL_TOL, class PIXELINDEX,
	class FRAMESIZE, PIXELINDEX PGW, PIXELINDEX PGH,
	class SORTERBITMASK, class PIXEL, class REFERENCEPIXEL,
	class REFERENCEFRAME>
MotionSearcher<PIXEL_NUM,DIM,PIXEL_TOL,PIXELINDEX,FRAMESIZE,
	PGW,PGH,SORTERBITMASK,PIXEL,REFERENCEPIXEL,
	REFERENCEFRAME>::MatchedPixelGroup::MatchedPixelGroup()
{
	// No match yet.
	m_tnSAD = 0;
	m_pGroup = NULL;
}



// Initializing constructor.
template <class PIXEL_NUM, int DIM, class PIXEL_TOL, class PIXELINDEX,
	class FRAMESIZE, PIXELINDEX PGW, PIXELINDEX PGH,
	class SORTERBITMASK, class PIXEL, class REFERENCEPIXEL,
	class REFERENCEFRAME>
MotionSearcher<PIXEL_NUM,DIM,PIXEL_TOL,PIXELINDEX,FRAMESIZE,
	PGW,PGH,SORTERBITMASK,PIXEL,REFERENCEPIXEL,
	REFERENCEFRAME>::MatchedPixelGroup::MatchedPixelGroup
	(Tolerance_t a_tnSAD,
	const typename SearchWindow_t::PixelGroup *a_pGroup)
: m_tnSAD (a_tnSAD), m_pGroup (a_pGroup)
{
	// Nothing else to do.
}



// Destructor.
template <class PIXEL_NUM, int DIM, class PIXEL_TOL, class PIXELINDEX,
	class FRAMESIZE, PIXELINDEX PGW, PIXELINDEX PGH,
	class SORTERBITMASK, class PIXEL, class REFERENCEPIXEL,
	class REFERENCEFRAME>
MotionSearcher<PIXEL_NUM,DIM,PIXEL_TOL,PIXELINDEX,FRAMESIZE,
	PGW,PGH,SORTERBITMASK,PIXEL,REFERENCEPIXEL,
	REFERENCEFRAME>::MatchedPixelGroup::~MatchedPixelGroup()
{
	// Nothing to do.
}



// A comparison operator, suitable for Set<>.
template <class PIXEL_NUM, int DIM, class PIXEL_TOL, class PIXELINDEX,
	class FRAMESIZE, PIXELINDEX PGW, PIXELINDEX PGH,
	class SORTERBITMASK, class PIXEL, class REFERENCEPIXEL,
	class REFERENCEFRAME>
inline bool
MotionSearcher<PIXEL_NUM,DIM,PIXEL_TOL,PIXELINDEX,FRAMESIZE,
	PGW,PGH,SORTERBITMASK,PIXEL,REFERENCEPIXEL,
	REFERENCEFRAME>::MatchedPixelGroup::SortBySAD::operator()
	(const MatchedPixelGroup &a_rLeft,
	const MatchedPixelGroup &a_rRight) const
{
	// Easy enough.
	return (a_rLeft.m_tnSAD < a_rRight.m_tnSAD);
}

#endif // THROTTLE_PIXELSORTER_WITH_SAD



// Apply this region to the new frame.
template <class PIXEL_NUM, int DIM, class PIXEL_TOL, class PIXELINDEX,
	class FRAMESIZE, PIXELINDEX PGW, PIXELINDEX PGH,
	class SORTERBITMASK, class PIXEL, class REFERENCEPIXEL,
	class REFERENCEFRAME>
void
MotionSearcher<PIXEL_NUM,DIM,PIXEL_TOL,PIXELINDEX,FRAMESIZE,
	PGW,PGH,SORTERBITMASK,PIXEL,REFERENCEPIXEL,REFERENCEFRAME>
	::ApplyRegionToNewFrame (Status_t &a_reStatus,
	const MovedRegion &a_rRegion)
{
	PIXELINDEX tnMotionX, tnMotionY;
		// The region's motion vector, i.e. the offset between
		// the new-frame pixel and the corresponding
		// reference-frame pixel.
	typename MovedRegion::ConstIterator itExtent;
		// An extent to apply to the new frame.
	PIXELINDEX x;
		// Used to loop through pixels.

	// Make sure they didn't start us off with an error.
	assert (a_reStatus == g_kNoError);

	// Get this region's motion vector.
	a_rRegion.GetMotionVector (tnMotionX, tnMotionY);
	
	// Loop through the region's extents, locate every pixel it
	// describes, and set it to the corresponding pixel in the
	// reference-frame representation of the new frame.
	for (itExtent = a_rRegion.Begin();
		 itExtent != a_rRegion.End();
		 ++itExtent)
	{
		// Get the next extent.
		const typename MovedRegion::Extent &rExtent = *itExtent;

		// Loop through the pixels it represents, set each
		// new-frame pixel to the corresponding reference-frame
		// pixel.
		for (x = rExtent.m_tnXStart; x < rExtent.m_tnXEnd; ++x)
		{
#ifdef PRINT_SEARCHBORDER
			if (m_pNewFrame->GetPixel (x, rExtent.m_tnY) != NULL
				&& m_pNewFrame->GetPixel (x, rExtent.m_tnY)
					->GetFrameReferences() != 1)
			{
				fprintf (stderr, "Pixel (%d,%d) already resolved\n",
					int (x), int (rExtent.m_tnY));
			}
#endif // PRINT_SEARCHBORDER

			// Make sure this new-frame pixel hasn't been
			// resolved yet.
			assert (m_pNewFrame->GetPixel (x, rExtent.m_tnY)
				== NULL
			|| m_pNewFrame->GetPixel (x, rExtent.m_tnY)
				->GetFrameReferences() == 1);
	
			// Get the corresponding reference-frame pixel.
			ReferencePixel_t *pReferencePixel
				= m_pReferenceFrame->GetPixel (x + tnMotionX,
					rExtent.m_tnY + tnMotionY);
			assert (pReferencePixel != NULL);
	
			// Set the new-frame pixel to this reference pixel.
			m_pNewFrame->SetPixel (x, rExtent.m_tnY,
				pReferencePixel);
	
			// Accumulate the new-frame value of this pixel.
			pReferencePixel->AddSample
				(m_pNewFramePixels[FRAMESIZE (rExtent.m_tnY)
				 * FRAMESIZE (m_tnWidth) + FRAMESIZE (x)]);
		}
	}

#ifdef USE_REFERENCEFRAMEPIXELS_ONCE

	// All of these reference pixels have been used.
	for (itExtent = a_rRegion.Begin();
		 itExtent != a_rRegion.End();
		 ++itExtent)
	{
		// Get the current extent.
		typename MovedRegion::Extent oExtent = *itExtent;

		// Move it along the motion vector.
		oExtent.m_tnY += tnMotionY;
		oExtent.m_tnXStart += tnMotionX;
		oExtent.m_tnXEnd += tnMotionX;

		// Make sure it's already in the frame.
		assert (oExtent.m_tnY >= 0 && oExtent.m_tnY < m_tnHeight
		&& oExtent.m_tnXStart >= 0 && oExtent.m_tnXEnd <= m_tnWidth);

		// Add it to our running total of used reference-pixels.
		m_oUsedReferencePixels.Union (a_reStatus, oExtent.m_tnY,
			oExtent.m_tnXStart, oExtent.m_tnXEnd);
		if (a_reStatus != g_kNoError)
			return;
	}

#endif // USE_REFERENCEFRAMEPIXELS_ONCE

	// Remove all pixel-groups containing used reference pixels from
	// the search window.
	m_oSearchWindow.Prune (a_rRegion, tnMotionX, tnMotionY);
}



#ifdef USE_SEARCH_BORDER

// Default constructor.
template <class PIXEL_NUM, int DIM, class PIXEL_TOL, class PIXELINDEX,
	class FRAMESIZE, PIXELINDEX PGW, PIXELINDEX PGH,
	class SORTERBITMASK, class PIXEL, class REFERENCEPIXEL,
	class REFERENCEFRAME>
MotionSearcher<PIXEL_NUM,DIM,PIXEL_TOL,PIXELINDEX,FRAMESIZE,
		PGW,PGH,SORTERBITMASK,PIXEL,REFERENCEPIXEL,
		REFERENCEFRAME>::SearchBorder::SearchBorder
		(MovedRegionSet &a_rsetRegions)
	: m_rsetRegions (a_rsetRegions)
{
	// Nothing else to do.
}



// Receive a completed region from the search-border.
template <class PIXEL_NUM, int DIM, class PIXEL_TOL, class PIXELINDEX,
	class FRAMESIZE, PIXELINDEX PGW, PIXELINDEX PGH,
	class SORTERBITMASK, class PIXEL, class REFERENCEPIXEL,
	class REFERENCEFRAME>
void
MotionSearcher<PIXEL_NUM,DIM,PIXEL_TOL,PIXELINDEX,FRAMESIZE,
	PGW,PGH,SORTERBITMASK,PIXEL,REFERENCEPIXEL,
	REFERENCEFRAME>::SearchBorder::OnCompletedRegion
	(Status_t &a_reStatus,
	typename SearchBorder::MovedRegion *a_pRegion)
{
	// Make sure they didn't start us off with an error.
	assert (a_reStatus == g_kNoError);

	// Make sure they gave us a completed region.
	assert (a_pRegion != NULL);

	// Put it in our list.
	if (a_pRegion->NumberOfPoints() <= 2 * PGW * PGH)
	{
		// This region is too small to be bothered with.
		// Just get rid of it.
		delete a_pRegion;
	}
	else
	{
#ifndef NDEBUG
		typename MovedRegionSet::InsertResult oInsertResult =
#endif // NDEBUG
		m_rsetRegions.Insert (a_reStatus, a_pRegion);
		if (a_reStatus != g_kNoError)
			return;
		assert (oInsertResult.m_bInserted);
	}
}



// Remove all regions that matched the current pixel-group, except for
// the single best one, flood-fill it, and apply it to the new frame.
template <class PIXEL_NUM, int DIM, class PIXEL_TOL, class PIXELINDEX,
	class FRAMESIZE, PIXELINDEX PGW, PIXELINDEX PGH,
	class SORTERBITMASK, class PIXEL, class REFERENCEPIXEL,
	class REFERENCEFRAME>
FRAMESIZE
MotionSearcher<PIXEL_NUM,DIM,PIXEL_TOL,PIXELINDEX,FRAMESIZE,
	PGW,PGH,SORTERBITMASK,PIXEL,REFERENCEPIXEL,
	REFERENCEFRAME>::SearchBorder_FloodFill (Status_t &a_reStatus,
	PIXELINDEX &a_rtnMotionX, PIXELINDEX &a_rtnMotionY)
{
	MovedRegion *pSurvivor;
		// The only region to survive pruning -- the biggest one, with
		// the shortest motion vector.

	// Get the best active region to apply to the frame now.
	pSurvivor = m_oSearchBorder.ChooseBestActiveRegion (a_reStatus);
	if (a_reStatus != g_kNoError)
		return 0;

	// Get the region's motion vector.
	pSurvivor->GetMotionVector (a_rtnMotionX, a_rtnMotionY);

	// Flood-fill this region, i.e. try to expand it as far as it'll
	// go while remaining contiguous.
	if (pSurvivor->NumberOfPoints() > 0)
	{
		m_oMatchThrottleFloodFillControl.SetupForFloodFill
			(a_rtnMotionX, a_rtnMotionY);
		pSurvivor->FloodFill (a_reStatus,
			m_oMatchThrottleFloodFillControl, true);
		if (a_reStatus != g_kNoError)
		{
			delete pSurvivor;
			return 0;
		}
	}

#ifdef PRINT_SEARCHBORDER
	if (frame == 61 && DIM == 2)
	{
	fprintf (stderr, "Flood-filled region:\n");
	PrintRegion (*pSurvivor);
	fprintf (stderr, "\n");
	}
#endif // PRINT_SEARCHBORDER

	// Apply this region to the new frame.
	if (pSurvivor->NumberOfPoints() > 0)
	{
		ApplyRegionToNewFrame (a_reStatus, *pSurvivor);
		if (a_reStatus != g_kNoError)
		{
			delete pSurvivor;
			return 0;
		}
	}

	// Clean up the region, return the number of points it had.
	FRAMESIZE tnPoints = pSurvivor->NumberOfPoints();
	delete pSurvivor;
	return tnPoints;
}

#endif // USE_SEARCH_BORDER



// Default constructor.
template <class PIXEL_NUM, int DIM, class PIXEL_TOL, class PIXELINDEX,
	class FRAMESIZE, PIXELINDEX PGW, PIXELINDEX PGH,
	class SORTERBITMASK, class PIXEL, class REFERENCEPIXEL,
	class REFERENCEFRAME>
MotionSearcher<PIXEL_NUM,DIM,PIXEL_TOL,PIXELINDEX,FRAMESIZE,
	PGW,PGH,SORTERBITMASK,PIXEL,REFERENCEPIXEL,
	REFERENCEFRAME>::ZeroMotionFloodFillControl
#ifdef ZERO_MOTION_FLOOD_FILL_WITH_BITMAP_REGIONS
	::ZeroMotionFloodFillControl()
#else // ZERO_MOTION_FLOOD_FILL_WITH_BITMAP_REGIONS
	::ZeroMotionFloodFillControl
		(typename BaseClass::Allocator &a_rAllocator)
	: BaseClass (a_rAllocator)
#endif // ZERO_MOTION_FLOOD_FILL_WITH_BITMAP_REGIONS
{
	// We don't know who we're working for yet.
	m_pMotionSearcher = NULL;
}



// Initializer.
template <class PIXEL_NUM, int DIM, class PIXEL_TOL, class PIXELINDEX,
	class FRAMESIZE, PIXELINDEX PGW, PIXELINDEX PGH,
	class SORTERBITMASK, class PIXEL, class REFERENCEPIXEL,
	class REFERENCEFRAME>
void
MotionSearcher<PIXEL_NUM,DIM,PIXEL_TOL,PIXELINDEX,FRAMESIZE,
	PGW,PGH,SORTERBITMASK,PIXEL,REFERENCEPIXEL,
	REFERENCEFRAME>::ZeroMotionFloodFillControl::Init
	(Status_t &a_reStatus, MotionSearcher *a_pMotionSearcher)
{
	// Make sure they didn't start us off with an error.
	assert (a_reStatus == g_kNoError);

	// Make sure they gave us a motion-searcher.
	assert (a_pMotionSearcher != NULL);

	// Initialize our base class.
	BaseClass::Init (a_reStatus
#ifdef ZERO_MOTION_FLOOD_FILL_WITH_BITMAP_REGIONS
							   , a_pMotionSearcher->m_tnWidth,
		a_pMotionSearcher->m_tnHeight
#endif // ZERO_MOTION_FLOOD_FILL_WITH_BITMAP_REGIONS
									 );
	if (a_reStatus != g_kNoError)
		return;

	// Remember which motion-searcher we're working for.
	m_pMotionSearcher = a_pMotionSearcher;
}



// Return true if the flood-fill should examine the given
// extent.
template <class PIXEL_NUM, int DIM, class PIXEL_TOL, class PIXELINDEX,
	class FRAMESIZE, PIXELINDEX PGW, PIXELINDEX PGH,
	class SORTERBITMASK, class PIXEL, class REFERENCEPIXEL,
	class REFERENCEFRAME>
bool
MotionSearcher<PIXEL_NUM,DIM,PIXEL_TOL,PIXELINDEX,FRAMESIZE,
	PGW,PGH,SORTERBITMASK,PIXEL,REFERENCEPIXEL,
	REFERENCEFRAME>::ZeroMotionFloodFillControl::ShouldUseExtent
	(typename ZeroMotionFloodFillControl::BaseClass::Extent &a_rExtent)
{
#ifdef ZERO_MOTION_FLOOD_FILL_WITH_BITMAP_REGIONS

	// Make sure the extent doesn't need to be clipped.
	assert (a_rExtent.m_tnY >= 0
		&& a_rExtent.m_tnY < m_pMotionSearcher->m_tnHeight
		&& a_rExtent.m_tnXStart >= 0
		&& a_rExtent.m_tnXStart < m_pMotionSearcher->m_tnWidth
		&& a_rExtent.m_tnXEnd > 0
		&& a_rExtent.m_tnXEnd <= m_pMotionSearcher->m_tnWidth);

#else // ZERO_MOTION_FLOOD_FILL_WITH_BITMAP_REGIONS

	// If this extent is completely off the screen, skip it.
	if (a_rExtent.m_tnY < 0
	|| a_rExtent.m_tnY >= m_pMotionSearcher->m_tnHeight
	|| a_rExtent.m_tnXStart >= m_pMotionSearcher->m_tnWidth
	|| a_rExtent.m_tnXEnd <= 0)
		return false;

	// If this extent is partially off the screen, clip it.
	if (a_rExtent.m_tnXStart < 0)
		a_rExtent.m_tnXStart = 0;
	if (a_rExtent.m_tnXEnd > m_pMotionSearcher->m_tnWidth)
		a_rExtent.m_tnXEnd = m_pMotionSearcher->m_tnWidth;

#endif // ZERO_MOTION_FLOOD_FILL_WITH_BITMAP_REGIONS

	// Let our caller know to use this extent.
	return true;
}



// Returns true if the given point should be included in the
// flood-fill.
template <class PIXEL_NUM, int DIM, class PIXEL_TOL, class PIXELINDEX,
	class FRAMESIZE, PIXELINDEX PGW, PIXELINDEX PGH,
	class SORTERBITMASK, class PIXEL, class REFERENCEPIXEL,
	class REFERENCEFRAME>
bool
MotionSearcher<PIXEL_NUM,DIM,PIXEL_TOL,PIXELINDEX,FRAMESIZE,
	PGW,PGH,SORTERBITMASK,PIXEL,REFERENCEPIXEL,
	REFERENCEFRAME>::ZeroMotionFloodFillControl::IsPointInRegion
	(PIXELINDEX a_tnX, PIXELINDEX a_tnY)
{
	// Make sure this pixel isn't resolved yet.
	assert (m_pMotionSearcher->m_pNewFrame->GetPixel (a_tnX, a_tnY)
		== NULL);

	// Get the pixels of interest.
	const Pixel_t &rNewPixel = m_pMotionSearcher->m_pNewFramePixels
		[a_tnY * m_pMotionSearcher->m_tnWidth + a_tnX];
	ReferencePixel_t *pRefPixel
		= m_pMotionSearcher->m_pReferenceFrame->GetPixel (a_tnX, a_tnY);

	// If the new pixel is close enough to the reference pixel, the
	// point is in the region.
	if (rNewPixel.IsWithinTolerance
		(pRefPixel->GetValue(), m_pMotionSearcher->m_tnZeroTolerance))
	{
		// Accumulate the value from the new frame.
		pRefPixel->AddSample (rNewPixel);

		// Store the pixel in the new reference frame.
		m_pMotionSearcher->m_pNewFrame->SetPixel (a_tnX, a_tnY,
			pRefPixel);

		// The point is in the region.
		return true;
	}

	// The point is not in the region.
	return false;
}



// Default constructor.
template <class PIXEL_NUM, int DIM, class PIXEL_TOL, class PIXELINDEX,
	class FRAMESIZE, PIXELINDEX PGW, PIXELINDEX PGH,
	class SORTERBITMASK, class PIXEL, class REFERENCEPIXEL,
	class REFERENCEFRAME>
MotionSearcher<PIXEL_NUM,DIM,PIXEL_TOL,PIXELINDEX,FRAMESIZE,
	PGW,PGH,SORTERBITMASK,PIXEL,REFERENCEPIXEL,
	REFERENCEFRAME>::MatchThrottleFloodFillControl
#ifdef MATCH_THROTTLE_FLOOD_FILL_WITH_BITMAP_REGIONS
	::MatchThrottleFloodFillControl()
#else // MATCH_THROTTLE_FLOOD_FILL_WITH_BITMAP_REGIONS
	::MatchThrottleFloodFillControl
		(typename BaseClass::Allocator &a_rAllocator)
	: BaseClass (a_rAllocator)
#endif // MATCH_THROTTLE_FLOOD_FILL_WITH_BITMAP_REGIONS
{
	// We don't know who we're working for yet.
	m_pMotionSearcher = NULL;
}



// Initializer.
template <class PIXEL_NUM, int DIM, class PIXEL_TOL, class PIXELINDEX,
	class FRAMESIZE, PIXELINDEX PGW, PIXELINDEX PGH,
	class SORTERBITMASK, class PIXEL, class REFERENCEPIXEL,
	class REFERENCEFRAME>
void
MotionSearcher<PIXEL_NUM,DIM,PIXEL_TOL,PIXELINDEX,FRAMESIZE,
	PGW,PGH,SORTERBITMASK,PIXEL,REFERENCEPIXEL,
	REFERENCEFRAME>::MatchThrottleFloodFillControl::Init
	(Status_t &a_reStatus, MotionSearcher *a_pMotionSearcher)
{
	// Make sure they didn't start us off with an error.
	assert (a_reStatus == g_kNoError);

	// Make sure they gave us a motion-searcher.
	assert (a_pMotionSearcher != NULL);

	// Initialize our base class.
	BaseClass::Init (a_reStatus
#ifdef MATCH_THROTTLE_FLOOD_FILL_WITH_BITMAP_REGIONS
							   , a_pMotionSearcher->m_tnWidth,
		a_pMotionSearcher->m_tnHeight
#endif // MATCH_THROTTLE_FLOOD_FILL_WITH_BITMAP_REGIONS
									 );
	if (a_reStatus != g_kNoError)
		return;

	// Remember which motion-searcher we're working for.
	m_pMotionSearcher = a_pMotionSearcher;
}



// Set up to to a flood-fill.
template <class PIXEL_NUM, int DIM, class PIXEL_TOL, class PIXELINDEX,
	class FRAMESIZE, PIXELINDEX PGW, PIXELINDEX PGH,
	class SORTERBITMASK, class PIXEL, class REFERENCEPIXEL,
	class REFERENCEFRAME>
void
MotionSearcher<PIXEL_NUM,DIM,PIXEL_TOL,PIXELINDEX,FRAMESIZE,
	PGW,PGH,SORTERBITMASK,PIXEL,REFERENCEPIXEL,
	REFERENCEFRAME>::MatchThrottleFloodFillControl::SetupForFloodFill
	(PIXELINDEX a_tnMotionX, PIXELINDEX a_tnMotionY)
{
	// Save the motion vector.
	m_tnMotionX = a_tnMotionX;
	m_tnMotionY = a_tnMotionY;
}



// Return true if the flood-fill should examine the given
// extent.
template <class PIXEL_NUM, int DIM, class PIXEL_TOL, class PIXELINDEX,
	class FRAMESIZE, PIXELINDEX PGW, PIXELINDEX PGH,
	class SORTERBITMASK, class PIXEL, class REFERENCEPIXEL,
	class REFERENCEFRAME>
bool
MotionSearcher<PIXEL_NUM,DIM,PIXEL_TOL,PIXELINDEX,FRAMESIZE,
	PGW,PGH,SORTERBITMASK,PIXEL,REFERENCEPIXEL,
	REFERENCEFRAME>::MatchThrottleFloodFillControl::ShouldUseExtent
	(typename Region_t::Extent &a_rExtent)
{
#ifdef MATCH_THROTTLE_FLOOD_FILL_WITH_BITMAP_REGIONS

	// Make sure the extent doesn't need to be clipped.
	assert (a_rExtent.m_tnY >= 0
		&& a_rExtent.m_tnY < m_pMotionSearcher->m_tnHeight
		&& a_rExtent.m_tnXStart >= 0
		&& a_rExtent.m_tnXStart < m_pMotionSearcher->m_tnWidth
		&& a_rExtent.m_tnXEnd > 0
		&& a_rExtent.m_tnXEnd <= m_pMotionSearcher->m_tnWidth);

	// If this extent (with its motion vector) is completely off the
	// screen, skip it.
	if (a_rExtent.m_tnY + m_tnMotionY < 0
	|| a_rExtent.m_tnY + m_tnMotionY >= m_pMotionSearcher->m_tnHeight
	|| a_rExtent.m_tnXStart + m_tnMotionX
		>= m_pMotionSearcher->m_tnWidth
	|| a_rExtent.m_tnXEnd + m_tnMotionX <= 0)
		return false;

	// If this extent (with its motion vector) is partially off the
	// screen, clip it.
	if (a_rExtent.m_tnXStart + m_tnMotionX < 0)
		a_rExtent.m_tnXStart = -m_tnMotionX;
	if (a_rExtent.m_tnXEnd + m_tnMotionX > m_pMotionSearcher->m_tnWidth)
		a_rExtent.m_tnXEnd = m_pMotionSearcher->m_tnWidth - m_tnMotionX;

#else // MATCH_THROTTLE_FLOOD_FILL_WITH_BITMAP_REGIONS

	// If this extent is completely off the screen, skip it.
	if (a_rExtent.m_tnY < 0
	|| a_rExtent.m_tnY >= m_pMotionSearcher->m_tnHeight
	|| a_rExtent.m_tnXStart >= m_pMotionSearcher->m_tnWidth
	|| a_rExtent.m_tnXEnd <= 0
	|| a_rExtent.m_tnY + m_tnMotionY < 0
	|| a_rExtent.m_tnY + m_tnMotionY >= m_pMotionSearcher->m_tnHeight
	|| a_rExtent.m_tnXStart + m_tnMotionX
		>= m_pMotionSearcher->m_tnWidth
	|| a_rExtent.m_tnXEnd + m_tnMotionX <= 0)
		return false;

	// If this extent is partially off the screen, clip it.
	if (a_rExtent.m_tnXStart + m_tnMotionX < 0)
		a_rExtent.m_tnXStart = -m_tnMotionX;
	if (a_rExtent.m_tnXEnd + m_tnMotionX > m_pMotionSearcher->m_tnWidth)
		a_rExtent.m_tnXEnd = m_pMotionSearcher->m_tnWidth - m_tnMotionX;
	if (a_rExtent.m_tnXStart < 0)
		a_rExtent.m_tnXStart = 0;
	if (a_rExtent.m_tnXEnd > m_pMotionSearcher->m_tnWidth)
		a_rExtent.m_tnXEnd = m_pMotionSearcher->m_tnWidth;

#endif // MATCH_THROTTLE_FLOOD_FILL_WITH_BITMAP_REGIONS

	// Let our caller know to use this extent.
	return true;
}



// Returns true if the given point should be included in the
// flood-fill.
template <class PIXEL_NUM, int DIM, class PIXEL_TOL, class PIXELINDEX,
	class FRAMESIZE, PIXELINDEX PGW, PIXELINDEX PGH,
	class SORTERBITMASK, class PIXEL, class REFERENCEPIXEL,
	class REFERENCEFRAME>
bool
MotionSearcher<PIXEL_NUM,DIM,PIXEL_TOL,PIXELINDEX,FRAMESIZE,
	PGW,PGH,SORTERBITMASK,PIXEL,REFERENCEPIXEL,
	REFERENCEFRAME>::MatchThrottleFloodFillControl::IsPointInRegion
	(PIXELINDEX a_tnX, PIXELINDEX a_tnY)
{
	// Get the new pixel, if any.
	ReferencePixel_t *pNewPixel = m_pMotionSearcher->m_pNewFrame
		->GetPixel (a_tnX, a_tnY);

	// We can potentially flood-fill this pixel if it's unresolved or
	// if it thinks it's new data.
	if (pNewPixel == NULL || pNewPixel->GetFrameReferences() == 1)
	{
		// Get the reference pixel's coordinates.
		PIXELINDEX tnRefX = a_tnX + m_tnMotionX;
		PIXELINDEX tnRefY = a_tnY + m_tnMotionY;

#ifdef USE_REFERENCEFRAMEPIXELS_ONCE
		// If the corresponding reference pixel hasn't been
		// used already, see if it matches our pixel.
		if (!m_pMotionSearcher->m_oUsedReferencePixels.DoesContainPoint
			(tnRefY, tnRefX))
		{
#endif // USE_REFERENCEFRAMEPIXELS_ONCE
			// If the new pixel is close enough to it, the point is in
			// the region.
			ReferencePixel_t *pRefPixel
				= m_pMotionSearcher->m_pReferenceFrame->GetPixel
					(tnRefX, tnRefY);
			const Pixel_t &rNewPixel = m_pMotionSearcher
				->m_pNewFramePixels[a_tnY
					* m_pMotionSearcher->m_tnWidth + a_tnX];
			if (rNewPixel.IsWithinTolerance (pRefPixel->GetValue(),
				 m_pMotionSearcher->m_tnTolerance))
			{
				// Let our caller know the point is in the region.
				return true;
			}
#ifdef USE_REFERENCEFRAMEPIXELS_ONCE
		}
#endif // USE_REFERENCEFRAMEPIXELS_ONCE
	}

	// Let our caller know the point is not in the region.
	return false;
}



#endif // __MOTION_SEARCHER_H__
