#ifndef __SEARCH_BORDER_H__
#define __SEARCH_BORDER_H__

// This file (C) 2004 Steven Boswell.  All rights reserved.
// Released to the public under the GNU General Public License.
// See the file COPYING for more information.

#include "config.h"
#include <assert.h>
#include "mjpeg_types.h"
#include "TemplateLib.hh"
#include "Limits.hh"
#include "DoublyLinkedList.hh"
#include "SetRegion2D.hh"

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



// The generic search-border class.  It's parameterized by the numeric
// type to use for pixel indices and a numeric type big enough to hold
// the product of the largest expected frame width/height.
// When constructed, it's configured with the size of the frame in which
// it operates, and the width/height of pixel groups to operate on.
//
// The search border keeps track of all regions on the border between
// the searched area and the not-yet-searched area.  It also constructs
// regions so that each one is contiguous, and not contiguous with any
// other region.  When no new pixel-group could possibly intersect a
// region, that region is removed from the border and handed back to
// the client.
//
// The meaning of the regions it keeps track of is abstract.  The
// original purpose was to help the motion-searcher keep track of moved
// regions, i.e. pixels in the new frame that match pixels in the
// reference frame, only moved.  But it could presumably be used for
// other purposes, e.g. if it assembled regions of pixels that had
// exactly one frame reference, it could be used to detect bit noise,
// film lint/scratches, or even LaserDisc rot.
//
// As the search-border proceeds through the frame, it maintains a list
// of all regions that intersect with the current pixel-group.  That
// way, when it comes time to add a new match, all regions that
// intersect it are already known.
template <class PIXELINDEX, class FRAMESIZE>
class SearchBorder
{
public:
	typedef SetRegion2D<PIXELINDEX,FRAMESIZE> Region_t;
		// How we use Region2D<>.

	class MovedRegion;
		// A moved region of pixels that has been detected.
		// Derived from Region_t; defined below.

	SearchBorder();
		// Default constructor.

	virtual ~SearchBorder();
		// Destructor.

	void Init (Status_t &a_reStatus, PIXELINDEX a_tnWidth,
			PIXELINDEX a_tnHeight, PIXELINDEX a_tnPGW,
			PIXELINDEX a_tnPGH);
		// Initializer.  Provide the dimension of the frame and the
		// dimension of pixel-groups.

	void StartFrame (Status_t &a_reStatus);
		// Initialize the search-border, i.e. start in the upper-left
		// corner.

	void MoveRight (Status_t &a_reStatus);
		// Move one pixel to the right, adding and removing regions from
		// the potentially-intersecting list.

	void MoveLeft (Status_t &a_reStatus);
		// Move one pixel to the left, adding and removing regions from
		// the potentially-intersecting list.

	void MoveDown (Status_t &a_reStatus);
		// Move down a line.  Find all regions that can no longer
		// be contiguous with new matches, and hand them back to the
		// client.  Then rotate the border structures, making the
		// least-recent current border into the last border, etc.

	inline FRAMESIZE NumberOfActiveRegions (void) const;
		// Return the number of regions that would intersect with the
		// current pixel-group.

	FRAMESIZE AddNewMatch (Status_t &a_reStatus,
			PIXELINDEX a_tnMotionX, PIXELINDEX a_tnMotionY);
		// Accept a new match for a pixel-group, with the given
		// motion-vector.  May cause regions under construction to get
		// merged together.
		// Returns the size of the region containing the current
		// pixel-group.

	MovedRegion *ChooseBestActiveRegion (Status_t &a_reStatus);
		// Remove all regions that matched the current pixel-group,
		// except for the best one, and return it.

	void FinishFrame (Status_t &a_reStatus);
		// Clean up the search border at the end of a frame, e.g. hand
		// all remaining regions back to the client.
	
	virtual void OnCompletedRegion (Status_t &a_reStatus,
			MovedRegion *a_pRegion) = 0;
		// Hand a completed region to our client.  Subclasses must
		// override this to describe how to do this.
	
	// A moved region of pixels that has been detected.
	// All extents are in the coordinate system of the new frame; that
	// makes it easy to unify/subtract regions without regard to their
	// motion vector.
	class MovedRegion : public Region_t
	{
	private:
		typedef Region_t BaseClass;
			// Keep track of who our base class is.

	public:
		MovedRegion (typename BaseClass::Allocator &a_rAlloc
				= BaseClass::Extents::Imp::sm_oNodeAllocator);
			// Default constructor.  Must be followed by Init().

		MovedRegion (Status_t &a_reStatus, typename BaseClass::Allocator
				&a_rAlloc = BaseClass::Extents::Imp::sm_oNodeAllocator);
			// Initializing constructor.  Creates an empty region.

		MovedRegion (Status_t &a_reStatus, const MovedRegion &a_rOther);
			// Copy constructor.

		void Init (Status_t &a_reStatus);
			// Initializer.  Must be called on default-constructed
			// regions.

		void Assign (Status_t &a_reStatus, const MovedRegion &a_rOther);
			// Make the current region a copy of the other region.

		virtual ~MovedRegion();
			// Destructor.

		inline void SetMotionVector (PIXELINDEX a_tnX,
				PIXELINDEX a_tnY);
			// Set the motion vector.

		inline void GetMotionVector (PIXELINDEX &a_rtnX,
				PIXELINDEX &a_rtnY) const;
			// Get the motion vector.

		// Comparison class, suitable for Set<>.
		class SortBySizeThenMotionVectorLength
		{
		public:
			inline bool operator() (const MovedRegion *a_pLeft,
				const MovedRegion *a_pRight) const;
		};

		inline FRAMESIZE GetSquaredMotionVectorLength (void) const;
			// Get the squared length of the motion vector.
			// Needed by SortBySizeThenMotionVectorLength.

	private:
		PIXELINDEX m_tnX, m_tnY;
			// The motion vector associated with this region.

		FRAMESIZE m_tnSquaredLength;
			// The squared length of the motion vector.
			// Used for sorting.
	};

private:
	PIXELINDEX m_tnWidth, m_tnHeight;
		// The dimension of each reference frame.

	PIXELINDEX m_tnPGW, m_tnPGH;
		// The dimension of pixel groups.

	PIXELINDEX m_tnX, m_tnY;
		// The index of the current pixel group.  Actually the index
		// of the top-left pixel in the current pixel group.  This
		// gets moved in a zigzag pattern, back and forth across the
		// frame and then down, until the end of the frame is reached.
	
	PIXELINDEX m_tnStepX;
		// Whether we're zigging or zagging.

	// A region under construction.  Contains a MovedRegion that's
	// known to be on the border between the searched area and the
	// not-yet-searched area.
	// If two regions being constructed get merged, one is modified to
	// point to the other's contained MovedRegion, and they're put into
	// a doubly-linked-list with each other.
	// When there are no more references to a region, and it has no
	// siblings in the doubly-linked list, that means the region is no
	// longer on the border, and can be handed back to the
	// search-border's client.
	class RegionUnderConstruction
		: public DoublyLinkedList<RegionUnderConstruction>
	{
	public:
		MovedRegion *m_pRegion;
			// The region being constructed.

		FRAMESIZE m_tnReferences;
			// The number of beginnings/endings of extents of this
			// region that are on the border.  When this goes to zero,
			// and we have no siblings in the doubly-linked-list, then
			// that means no other matches could possibly be added to
			// the region, and m_pRegion will get handed back to the
			// search-border's client.

		RegionUnderConstruction();
			// Default constructor.

		~RegionUnderConstruction();
			// Destructor.

	private:
		typedef DoublyLinkedList<RegionUnderConstruction> BaseClass;
			// Keep track of who our base class is.

#ifndef NDEBUG

		// Count the number of region objects in existence.
		private:
			static uint32_t sm_ulInstances;
		public:
			static uint32_t GetInstances (void)
				{ return sm_ulInstances; }
	
#endif // NDEBUG
	};

	// A class that keeps track of region extents on the border between
	// the searched area and the not-yet-searched area, i.e. the only
	// regions that have a chance of growing.
	class BorderExtentBoundary
	{
	public:
		PIXELINDEX m_tnIndex;
			// The index of the endpoint of an extent.

		PIXELINDEX m_tnLine;
			// The vertical line on which this endpoint resides.
			// Used to quickly tell current-border endpoints apart from
			// last-border endpoints, and for no other reason.

		bool m_bIsEnding;
			// false if this is the beginning of an extent, true if
			// it's the end of an extent.

		BorderExtentBoundary *m_pCounterpart;
			// The ending to go with this beginning, or the beginning to
			// go with this ending.

		RegionUnderConstruction *m_pRegion;
			// The region with the given extent.

		PIXELINDEX m_tnMotionX, m_tnMotionY;
			// The region's motion vector.  Copied here so that our sort
			// order doesn't depend on m_pRegion's contents, i.e. so
			// that we thrash memory less.

		BorderExtentBoundary();
			// Default constructor.

		BorderExtentBoundary (PIXELINDEX a_tnIndex, PIXELINDEX a_tnLine,
				bool a_bIsEnding, RegionUnderConstruction *a_pRegion);
			// Initializing constructor.

		~BorderExtentBoundary();
			// Destructor.

#ifndef NDEBUG
		bool operator == (const BorderExtentBoundary &a_rOther) const;
			// Equality operator.
#endif // NDEBUG

		// Comparison class, suitable for Set<>.
		class SortByIndexThenTypeThenMotionVectorThenRegionAddress
		{
		public:
			inline bool operator() (const BorderExtentBoundary &a_rLeft,
				const BorderExtentBoundary &a_rRight) const;
		};

		// Comparison class, suitable for Set<>.
		class SortByMotionVectorThenTypeThenRegionAddress
		{
		public:
			inline bool operator() (const BorderExtentBoundary &a_rLeft,
				const BorderExtentBoundary &a_rRight) const;
		};
	};

	typedef Set<BorderExtentBoundary, typename BorderExtentBoundary
		::SortByIndexThenTypeThenMotionVectorThenRegionAddress>
		BorderExtentBoundarySet;
	BorderExtentBoundarySet m_setBorderStartpoints,
			m_setBorderEndpoints;
		// The borders, i.e. the startpoint/endpoints for every
		// region under construction, for every line in the current
		// pixel-group's vertical extent.
	
	typename BorderExtentBoundarySet::ConstIterator
			*m_paitBorderStartpoints,
			*m_paitBorderEndpoints;
		// The next last/current-border startpoints/endpoints whose
		// regions will be added or removed, when we move left or right.
		// (m_tnPGH + 1 iterators allocated for each.)

	typedef Set<BorderExtentBoundary, typename BorderExtentBoundary
		::SortByMotionVectorThenTypeThenRegionAddress>
		IntersectingRegionsSet;
	IntersectingRegionsSet m_setBorderRegions;
		// All regions that could possibly intersect the current pixel
		// group, should a match be found.  Sorted by motion vector,
		// since matches must be added to a region with the exact same
		// motion vector.  There may be more than one such region; that
		// means the current match causes those regions to be contiguous
		// and thus they will get merged together.
		// (Note that this set is also sorted by type, i.e. whether
		// it's a beginning or end.  We only put beginnings into this
		// set.  This extra sort criteria is used to help us find the
		// range of regions that all have the same motion vector, to let
		// us set up an umambiguous upper-bound for the search.)

#ifndef NDEBUG
public:
	static uint32_t GetRegionUnderConstructionCount (void)
		{ return RegionUnderConstruction::GetInstances(); }
#endif // NDEBUG
};



// Default constructor.
template <class PIXELINDEX, class FRAMESIZE>
SearchBorder<PIXELINDEX,FRAMESIZE>::SearchBorder()
{
	// No frame dimensions yet.
	m_tnWidth = m_tnHeight = PIXELINDEX (0);

	// No active search yet.
	m_tnX = m_tnY = m_tnStepX = PIXELINDEX (0);
}



// Destructor.
template <class PIXELINDEX, class FRAMESIZE>
SearchBorder<PIXELINDEX,FRAMESIZE>::~SearchBorder()
{
	// Make sure our client didn't stop in the middle of a frame.
	assert (m_setBorderStartpoints.Size() == 0);
	assert (m_setBorderEndpoints.Size() == 0);
	assert (m_setBorderRegions.Size() == 0);

	// Free up our arrays of iterators.
	delete[] m_paitBorderStartpoints;
	delete[] m_paitBorderEndpoints;
}



// Initializer.
template <class PIXELINDEX, class FRAMESIZE>
void
SearchBorder<PIXELINDEX,FRAMESIZE>::Init (Status_t &a_reStatus,
	PIXELINDEX a_tnWidth, PIXELINDEX a_tnHeight, PIXELINDEX a_tnPGW,
	PIXELINDEX a_tnPGH)
{
	// Make sure they didn't start us off with an error.
	assert (a_reStatus == g_kNoError);

	// Make sure the width & height are reasonable.
	assert (a_tnWidth > PIXELINDEX (0));
	assert (a_tnHeight > PIXELINDEX (0));

	// Initialize the sets that implement our border-regions.
	m_setBorderStartpoints.Init (a_reStatus, true);
	if (a_reStatus != g_kNoError)
		return;
	m_setBorderEndpoints.Init (a_reStatus, true);
	if (a_reStatus != g_kNoError)
		return;
	m_setBorderRegions.Init (a_reStatus, false);
	if (a_reStatus != g_kNoError)
		return;

	// Allocate space for our iterators into the startpoint/endpoint
	// sets.  (These move left/right/down with the current pixel-group,
	// and run over regions that get added/removed from the border
	// regions set.)
	m_paitBorderStartpoints = new typename
		BorderExtentBoundarySet::ConstIterator[a_tnPGH + 1];
	m_paitBorderEndpoints = new typename
		BorderExtentBoundarySet::ConstIterator[a_tnPGH + 1];
	if (m_paitBorderStartpoints == NULL
		|| m_paitBorderEndpoints == NULL)
	{
		delete[] m_paitBorderStartpoints;
		delete[] m_paitBorderEndpoints;
		return;
	}

	// Finally, store our parameters.
	m_tnWidth = a_tnWidth;
	m_tnHeight = a_tnHeight;
	m_tnPGW = a_tnPGW;
	m_tnPGH = a_tnPGH;
}



// Default constructor.  Must be followed by Init().
template <class PIXELINDEX, class FRAMESIZE>
SearchBorder<PIXELINDEX,FRAMESIZE>::MovedRegion::MovedRegion
		(typename BaseClass::Allocator &a_rAlloc)
	: BaseClass (a_rAlloc)
{
	// No motion-vector yet.
	m_tnX = m_tnY = PIXELINDEX (0);
	m_tnSquaredLength = FRAMESIZE (0);
}



// Initializing constructor.  Creates an empty region.
template <class PIXELINDEX, class FRAMESIZE>
SearchBorder<PIXELINDEX,FRAMESIZE>::MovedRegion::MovedRegion
		(Status_t &a_reStatus, typename BaseClass::Allocator &a_rAlloc)
	: BaseClass (a_rAlloc)
{
	// Make sure they didn't start us off with an error.
	assert (a_reStatus == g_kNoError);

	// No motion-vector yet.
	m_tnX = m_tnY = PIXELINDEX (0);
	m_tnSquaredLength = FRAMESIZE (0);

	// Initialize ourselves.
	Init (a_reStatus);
	if (a_reStatus != g_kNoError)
		return;
}



// Copy constructor.
template <class PIXELINDEX, class FRAMESIZE>
SearchBorder<PIXELINDEX,FRAMESIZE>::MovedRegion::MovedRegion
		(Status_t &a_reStatus, const MovedRegion &a_rOther)
	: BaseClass (a_reStatus, a_rOther)
{
	// No motion-vector yet.
	m_tnX = m_tnY = PIXELINDEX (0);
	m_tnSquaredLength = FRAMESIZE (0);

	// If copying our base class failed, leave.
	if (a_reStatus != g_kNoError)
		return;

	// Copy the motion vector.
	m_tnX = a_rOther.m_tnX;
	m_tnY = a_rOther.m_tnY;
	m_tnSquaredLength = a_rOther.m_tnSquaredLength;
}



// Initializer.  Must be called on default-constructed regions.
template <class PIXELINDEX, class FRAMESIZE>
void
SearchBorder<PIXELINDEX,FRAMESIZE>::MovedRegion::Init
	(Status_t &a_reStatus)
{
	// Make sure they didn't start us off with an error.
	assert (a_reStatus == g_kNoError);

	// Initialize the base-class.
	BaseClass::Init (a_reStatus);
	if (a_reStatus != g_kNoError)
		return;
}



// Make the current region a copy of the other region.
template <class PIXELINDEX, class FRAMESIZE>
void
SearchBorder<PIXELINDEX,FRAMESIZE>::MovedRegion::Assign
	(Status_t &a_reStatus, const MovedRegion &a_rOther)
{
	// Make sure they didn't start us off with an error.
	assert (a_reStatus == g_kNoError);

	// Assign the base class.
	BaseClass::Assign (a_reStatus, a_rOther);
	if (a_reStatus != g_kNoError)
		return;

	// Copy the motion vector.
	m_tnX = a_rOther.m_tnX;
	m_tnY = a_rOther.m_tnY;
	m_tnSquaredLength = a_rOther.m_tnSquaredLength;
}



// Destructor.
template <class PIXELINDEX, class FRAMESIZE>
SearchBorder<PIXELINDEX,FRAMESIZE>::MovedRegion::~MovedRegion()
{
	// Nothing additional to do.
}



// Set the motion vector.
template <class PIXELINDEX, class FRAMESIZE>
inline void
SearchBorder<PIXELINDEX,FRAMESIZE>::MovedRegion::SetMotionVector
	(PIXELINDEX a_tnX, PIXELINDEX a_tnY)
{
	// Set the motion vector.
	m_tnX = a_tnX;
	m_tnY = a_tnY;

	// Calculate the square of the vector's length.  (It's used for
	// sorting, and we don't want to recalculate it on every
	// comparison.)
	m_tnSquaredLength = FRAMESIZE (m_tnX) * FRAMESIZE (m_tnX)
		+ FRAMESIZE (m_tnY) * FRAMESIZE (m_tnY);
}



// Get the motion vector.
template <class PIXELINDEX, class FRAMESIZE>
inline void
SearchBorder<PIXELINDEX,FRAMESIZE>::MovedRegion::GetMotionVector
	(PIXELINDEX &a_rtnX, PIXELINDEX &a_rtnY) const
{
	// Easy enough.
	a_rtnX = m_tnX;
	a_rtnY = m_tnY;
}



// Comparison operator.
template <class PIXELINDEX, class FRAMESIZE>
inline bool
SearchBorder<PIXELINDEX,FRAMESIZE>::MovedRegion
	::SortBySizeThenMotionVectorLength::operator()
	(const MovedRegion *a_pLeft, const MovedRegion *a_pRight) const
{
	FRAMESIZE nLeftPoints, nRightPoints;
		// The number of points in each region.
	FRAMESIZE tnLeftLen, tnRightLen;
		// The (squared) length of each motion vector.

	// Make sure they gave us some regions to compare.
	assert (a_pLeft != NULL);
	assert (a_pRight != NULL);

	// First, compare by the number of points in each region.
	// Sort bigger regions first.
	nLeftPoints = a_pLeft->NumberOfPoints();
	nRightPoints = a_pRight->NumberOfPoints();
	if (nLeftPoints > nRightPoints)
		return true;
	if (nLeftPoints < nRightPoints)
		return false;

	// Then compare on motion vector length.
	// Sort smaller vectors first.
	tnLeftLen = a_pLeft->GetSquaredMotionVectorLength();
	tnRightLen = a_pRight->GetSquaredMotionVectorLength();
	if (tnLeftLen < tnRightLen)
		return true;
	// if (tnLeftLen >= tnRightLen)
		return false;
}



// Get the squared length of the motion vector.
template <class PIXELINDEX, class FRAMESIZE>
inline FRAMESIZE
SearchBorder<PIXELINDEX,FRAMESIZE>::MovedRegion
	::GetSquaredMotionVectorLength (void) const
{
	// Easy enough.
	return m_tnSquaredLength;
}



// Default constructor.
template <class PIXELINDEX, class FRAMESIZE>
SearchBorder<PIXELINDEX,FRAMESIZE>::RegionUnderConstruction
	::RegionUnderConstruction()
{
#ifndef NDEBUG
	// One more instance.
	++sm_ulInstances;
#endif // NDEBUG

	// Fill in the blanks.
	m_pRegion = NULL;
	m_tnReferences = FRAMESIZE (0);
}



// Destructor.
template <class PIXELINDEX, class FRAMESIZE>
SearchBorder<PIXELINDEX,FRAMESIZE>::RegionUnderConstruction
	::~RegionUnderConstruction()
{
#ifndef NDEBUG
	// One less instance.
	--sm_ulInstances;
#endif // NDEBUG

	// Make sure all references have been removed.
	assert (m_pRegion == NULL);
	assert (m_tnReferences == FRAMESIZE (0));
}



#ifndef NDEBUG

template <class PIXELINDEX, class FRAMESIZE>
uint32_t
SearchBorder<PIXELINDEX,FRAMESIZE>::RegionUnderConstruction
	::sm_ulInstances;

#endif // NDEBUG



// Default constructor.
template <class PIXELINDEX, class FRAMESIZE>
SearchBorder<PIXELINDEX,FRAMESIZE>::BorderExtentBoundary
	::BorderExtentBoundary()
{
	// Fill in the blanks.
	m_tnIndex = m_tnLine = PIXELINDEX (0);
	m_bIsEnding = false;
	m_pCounterpart = NULL;
	m_pRegion = NULL;
	m_tnMotionX = m_tnMotionY = PIXELINDEX (0);
}



// Initializing constructor.
template <class PIXELINDEX, class FRAMESIZE>
SearchBorder<PIXELINDEX,FRAMESIZE>::BorderExtentBoundary
	::BorderExtentBoundary (PIXELINDEX a_tnIndex,
	PIXELINDEX a_tnLine, bool a_bIsEnding,
	RegionUnderConstruction *a_pRegion)
{
	// Make sure they gave us a region.
	assert (a_pRegion != NULL);
	assert (a_pRegion->m_pRegion != NULL);

	// Fill in the blanks.
	m_tnIndex = a_tnIndex;
	m_tnLine = a_tnLine;
	m_bIsEnding = a_bIsEnding;
	m_pCounterpart = NULL;
	m_pRegion = a_pRegion;
	a_pRegion->m_pRegion->GetMotionVector (m_tnMotionX, m_tnMotionY);
}



// Destructor.
template <class PIXELINDEX, class FRAMESIZE>
SearchBorder<PIXELINDEX,FRAMESIZE>::BorderExtentBoundary
	::~BorderExtentBoundary()
{
	// Nothing to do.
}



#ifndef NDEBUG

// Equality operator.
template <class PIXELINDEX, class FRAMESIZE>
bool
SearchBorder<PIXELINDEX,FRAMESIZE>::BorderExtentBoundary
	::operator == (const BorderExtentBoundary &a_rOther) const
{
	// Compare ourselves, field by field.
	return (m_tnIndex == a_rOther.m_tnIndex
		&& m_tnLine == a_rOther.m_tnLine
		&& m_bIsEnding == a_rOther.m_bIsEnding
		&& m_pCounterpart == a_rOther.m_pCounterpart
		&& m_pRegion == a_rOther.m_pRegion
		&& m_tnMotionX == a_rOther.m_tnMotionX
		&& m_tnMotionY == a_rOther.m_tnMotionY);
}

#endif // NDEBUG



// Comparison operator.
template <class PIXELINDEX, class FRAMESIZE>
inline bool
SearchBorder<PIXELINDEX,FRAMESIZE> ::BorderExtentBoundary
	::SortByIndexThenTypeThenMotionVectorThenRegionAddress
	::operator() (const BorderExtentBoundary &a_rLeft,
	const BorderExtentBoundary &a_rRight) const
{
	// First, sort by the boundary's pixel line.
	if (a_rLeft.m_tnLine < a_rRight.m_tnLine)
		return true;
	if (a_rLeft.m_tnLine > a_rRight.m_tnLine)
		return false;

	// Then sort by the boundary's pixel index.
	if (a_rLeft.m_tnIndex < a_rRight.m_tnIndex)
		return true;
	if (a_rLeft.m_tnIndex > a_rRight.m_tnIndex)
		return false;

#if 0
	// (Not needed: startpoints & endpoints are in separate sets now.)
	// Then sort beginnings before endings.
	if (!a_rLeft.m_bIsEnding && a_rRight.m_bIsEnding)
		return true;
	if (a_rLeft.m_bIsEnding && !a_rRight.m_bIsEnding)
		return false;
#endif

	// Sort next by motion vector.  (It doesn't matter how the sort
	// order is defined from the motion vectors, just that one is
	// defined.)
	if (a_rLeft.m_tnMotionX < a_rRight.m_tnMotionX)
		return true;
	if (a_rLeft.m_tnMotionX > a_rRight.m_tnMotionX)
		return false;
	if (a_rLeft.m_tnMotionY < a_rRight.m_tnMotionY)
		return true;
	if (a_rLeft.m_tnMotionY > a_rRight.m_tnMotionY)
		return false;

	// Finally, disambiguate by region address.
	if (a_rLeft.m_pRegion < a_rRight.m_pRegion)
		return true;
	// if (a_rLeft.m_pRegion >= a_rRight.m_pRegion)
		return false;
}



// Comparison operator.
template <class PIXELINDEX, class FRAMESIZE>
inline bool
SearchBorder<PIXELINDEX,FRAMESIZE>::BorderExtentBoundary
	::SortByMotionVectorThenTypeThenRegionAddress::operator()
	(const BorderExtentBoundary &a_rLeft,
	const BorderExtentBoundary &a_rRight) const
{
	// Sort by motion vector.  (It doesn't matter how the sort
	// order is defined from the motion vectors, just that one is
	// defined.)
	if (a_rLeft.m_tnMotionX < a_rRight.m_tnMotionX)
		return true;
	if (a_rLeft.m_tnMotionX > a_rRight.m_tnMotionX)
		return false;
	if (a_rLeft.m_tnMotionY < a_rRight.m_tnMotionY)
		return true;
	if (a_rLeft.m_tnMotionY > a_rRight.m_tnMotionY)
		return false;

	// Next, sort beginnings before endings.
	if (!a_rLeft.m_bIsEnding && a_rRight.m_bIsEnding)
		return true;
	if (a_rLeft.m_bIsEnding && !a_rRight.m_bIsEnding)
		return false;

	// Next, sort by index.  (Regions may have more than one extent
	// on a border.)
	if (a_rLeft.m_tnIndex < a_rRight.m_tnIndex)
		return true;
	if (a_rLeft.m_tnIndex > a_rRight.m_tnIndex)
		return false;

	// Next, sort by lines.  (The same region may have extents on
	// multiple lines, and this also matches the order in the
	// startpoints/endpoints sets, so that searches take maximum
	// advantage of the search finger.)
	if (a_rLeft.m_tnLine < a_rRight.m_tnLine)
		return true;
	if (a_rLeft.m_tnLine > a_rRight.m_tnLine)
		return false;
	
	// Finally, disambiguate by region address.
	if (a_rLeft.m_pRegion < a_rRight.m_pRegion)
		return true;
	//if (a_rLeft.m_pRegion >= a_rRight.m_pRegion)
		return false;
}



// Initialize the search-border, i.e. start in the upper-left corner.
template <class PIXELINDEX, class FRAMESIZE>
void
SearchBorder<PIXELINDEX,FRAMESIZE>::StartFrame (Status_t &a_reStatus)
{
	// Make sure they didn't start us off with an error.
	assert (a_reStatus == g_kNoError);

	// Make sure the borders are empty.
	assert (m_setBorderStartpoints.Size() == 0);
	assert (m_setBorderEndpoints.Size() == 0);
	assert (m_setBorderRegions.Size() == 0);

	// Set up our iterators into the borders.
	for (int i = 0; i <= m_tnPGH; ++i)
	{
		m_paitBorderStartpoints[i] = m_setBorderStartpoints.End();
		m_paitBorderEndpoints[i] = m_setBorderEndpoints.End();
	}

	// Start in the upper-left corner, and prepare to go right.
	m_tnX = m_tnY = PIXELINDEX (0);
	m_tnStepX = PIXELINDEX (1);
}



// Move one pixel to the right, adding and removing regions from
// the potentially-intersecting list.
template <class PIXELINDEX, class FRAMESIZE>
void
SearchBorder<PIXELINDEX,FRAMESIZE>::MoveRight (Status_t &a_reStatus)
{
	PIXELINDEX tnI;
		// Used to loop through iterators.
	typename IntersectingRegionsSet::Iterator itRemove;
		// An item being removed from the possibly-intersecting set.

	// Make sure they didn't start us off with an error.
	assert (a_reStatus == g_kNoError);

	// Make sure we knew we were moving right.
	assert (m_tnStepX == 1);

#ifdef PRINT_SEARCHBORDER
	if (frame == 61 && m_setBorderRegions.Size() > 0)
	{
		fprintf (stderr, "Here's what SearchBorder::MoveRight() "
			"starts with (x %d, y %d):\n", int (m_tnX), int (m_tnY));
		for (itRemove = m_setBorderRegions.Begin();
			 itRemove != m_setBorderRegions.End();
			 ++itRemove)
		{
			fprintf (stderr, "\t(%d,%d), motion vector (%d,%d)\n",
				(*itRemove).m_tnIndex, (*itRemove).m_tnLine,
				(*itRemove).m_tnMotionX, (*itRemove).m_tnMotionY);
		}
	}
#endif

	// All active regions with a current-border endpoint at old X, and
	// all active regions with a last-border endpoint at old X + 1, are
	// no longer active.
	for (tnI = ((m_tnY > 0) ? 0 : 1); tnI <= m_tnPGH; ++tnI)
	{
		// Get the endpoint to search along.
		typename BorderExtentBoundarySet::ConstIterator
			&ritEndpoint = m_paitBorderEndpoints[tnI];

		// Make sure it's right where we expect it to be.
		assert (ritEndpoint == m_setBorderEndpoints.End()
			|| (*ritEndpoint).m_tnLine > m_tnY + tnI - PIXELINDEX (1)
			|| ((*ritEndpoint).m_tnLine == m_tnY + tnI - PIXELINDEX (1)
				&& (*ritEndpoint).m_tnIndex
					>= (m_tnX + ((tnI == 0) ? 1 : 0))));

		// Remove all active regions that have endpoints at this index.
		while (ritEndpoint != m_setBorderEndpoints.End()
		&& (*ritEndpoint).m_tnLine == m_tnY + tnI - PIXELINDEX (1)
		&& (*ritEndpoint).m_tnIndex == (m_tnX + ((tnI == 0) ? 1 : 0)))
		{
			// Find the endpoint's corresponding startpoint.
			const BorderExtentBoundary &rHere
				= *((*ritEndpoint).m_pCounterpart);
	
#ifdef PRINT_SEARCHBORDER
			if (frame == 61)
			fprintf (stderr, "Found current-border endpoint, remove "
				"active-border region (%d,%d), "
					"motion vector (%d,%d)\n",
				rHere.m_tnIndex, rHere.m_tnLine,
				rHere.m_tnMotionX, rHere.m_tnMotionY);
#endif
	
			// Now find it & remove it.
			itRemove = m_setBorderRegions.Find (rHere);
			assert (itRemove != m_setBorderRegions.End());
			m_setBorderRegions.Erase (itRemove);
	
			// Move to the next endpoint.
			++ritEndpoint;
		}
	}

	// All active regions with a current-border startpoint at
	// new X + m_tnPGW, and all active regions with a last-border
	// startpoint at new X + m_tnPGW - 1, are now active.
	for (tnI = ((m_tnY > 0) ? 0 : 1); tnI <= m_tnPGH; ++tnI)
	{
		// Get the startpoint to search along.
		typename BorderExtentBoundarySet::ConstIterator
			&ritStartpoint = m_paitBorderStartpoints[tnI];

		// Make sure it's right where we expect it to be.
		assert (ritStartpoint
			== m_setBorderStartpoints.End()
		|| (*ritStartpoint).m_tnLine > m_tnY + tnI - PIXELINDEX (1)
		|| ((*ritStartpoint).m_tnLine == m_tnY + tnI - PIXELINDEX (1)
			&& (*ritStartpoint).m_tnIndex
				>= (m_tnX + m_tnPGW + ((tnI == 0) ? 0 : 1))));

		// Add all active regions that have startpoints at this index.
		while (ritStartpoint != m_setBorderStartpoints.End()
		&& (*ritStartpoint).m_tnLine == m_tnY + tnI - PIXELINDEX (1)
		&& (*ritStartpoint).m_tnIndex
			== (m_tnX + m_tnPGW + ((tnI == 0) ? 0 : 1)))
		{
			const BorderExtentBoundary &rHere = *ritStartpoint;

#ifdef PRINT_SEARCHBORDER
			if (frame == 61)
			{
			fprintf (stderr, "Add active-border region (%d,%d), "
					"motion vector (%d,%d)\n",
				rHere.m_tnIndex, rHere.m_tnLine,
				rHere.m_tnMotionX, rHere.m_tnMotionY);
			}
#endif

#ifndef NDEBUG
			typename IntersectingRegionsSet::InsertResult oInsertResult=
#endif // NDEBUG
				m_setBorderRegions.Insert (a_reStatus, rHere);
			if (a_reStatus != g_kNoError)
				return;
			assert (oInsertResult.m_bInserted);
			
			// Move to the next startpoint.
			++ritStartpoint;
		}
	}

	// Finally, move one step to the right.
	++m_tnX;
}



// Move one pixel to the left, adding and removing regions from
// the potentially-intersecting list.
template <class PIXELINDEX, class FRAMESIZE>
void
SearchBorder<PIXELINDEX,FRAMESIZE>::MoveLeft (Status_t &a_reStatus)
{
	int tnI;
		// Used to loop through iterators.
	typename IntersectingRegionsSet::Iterator itRemove;
		// An item being removed from the possibly-intersecting set.

	// Make sure they didn't start us off with an error.
	assert (a_reStatus == g_kNoError);

	// Make sure we knew we were moving left.
	assert (m_tnStepX == -1);

#ifdef PRINT_SEARCHBORDER
	if (frame == 61 && m_setBorderRegions.Size() > 0)
	{
		fprintf (stderr, "Here's what SearchBorder::MoveLeft() "
			"starts with (x %d, y %d):\n", int (m_tnX), int (m_tnY));
		for (itRemove = m_setBorderRegions.Begin();
			 itRemove != m_setBorderRegions.End();
			 ++itRemove)
		{
			fprintf (stderr, "\t(%d,%d), motion vector (%d,%d)\n",
				(*itRemove).m_tnIndex, (*itRemove).m_tnLine,
				(*itRemove).m_tnMotionX, (*itRemove).m_tnMotionY);
		}
	}
#endif

	// All active regions with a current-border startpoint at
	// old X + m_tnPGW, and all active regions with a last-border
	// startpoint at old X + m_tnPGW - 1, are no longer active.
	for (tnI = ((m_tnY > 0) ? 0 : 1); tnI <= m_tnPGH; ++tnI)
	{
		// Get the current startpoint.
		typename BorderExtentBoundarySet::ConstIterator &ritBorder
			= m_paitBorderStartpoints[tnI];

#ifndef NDEBUG
		// Make sure it's right where we want to be.
		{
			typename BorderExtentBoundarySet::ConstIterator itPrev
				= ritBorder;
			--itPrev;
			assert (ritBorder == m_setBorderStartpoints.End()
			|| (*ritBorder).m_tnLine > m_tnY + tnI - PIXELINDEX (1)
			|| ((*ritBorder).m_tnLine == m_tnY + tnI - PIXELINDEX (1)
				&& ((*ritBorder).m_tnIndex
						> (m_tnX + m_tnPGW - ((tnI == 0) ? 1 : 0))
					&& (itPrev == m_setBorderStartpoints.End()
						|| (*itPrev).m_tnLine
							< m_tnY + tnI - PIXELINDEX (1)
						|| ((*itPrev).m_tnLine
							== m_tnY + tnI - PIXELINDEX (1)
							&& (*itPrev).m_tnIndex
								<= (m_tnX + m_tnPGW
									- ((tnI == 0) ? 1 : 0)))))));
		}
#endif // NDEBUG

		// Remove all active regions that have startpoints at this
		// index.
		while ((--ritBorder) != m_setBorderStartpoints.End()
		&& (*ritBorder).m_tnLine == m_tnY + tnI - PIXELINDEX (1)
		&& (*ritBorder).m_tnIndex
			== (m_tnX + m_tnPGW - ((tnI == 0) ? 1 : 0)))
		{
			const BorderExtentBoundary &rHere = *ritBorder;
	
#ifdef PRINT_SEARCHBORDER
			if (frame == 61)
			{
			fprintf (stderr, "Found current-border startpoint, remove "
					"active-border region (%d,%d), "
					"motion vector (%d,%d)\n",
				rHere.m_tnIndex, rHere.m_tnLine,
				rHere.m_tnMotionX, rHere.m_tnMotionY);
			}
#endif
	
			itRemove = m_setBorderRegions.Find (rHere);
	
#ifdef PRINT_SEARCHBORDER
			if (frame == 61 && itRemove == m_setBorderRegions.End())
			{
				fprintf (stderr, "NOT FOUND!\n"
					"Here's what was found:\n");
				for (itRemove = m_setBorderRegions.Begin();
					 itRemove != m_setBorderRegions.End();
					 ++itRemove)
				{
					fprintf (stderr, "\t(%d,%d), "
							"motion vector (%d,%d)\n",
						(*itRemove).m_tnIndex, (*itRemove).m_tnLine,
						(*itRemove).m_tnMotionX,
						(*itRemove).m_tnMotionY);
				}
			}
#endif
	
			assert (itRemove != m_setBorderRegions.End());
			m_setBorderRegions.Erase (itRemove);
		}
		++ritBorder;
	}

	// All active regions with a current-border endpoint at new X, and
	// all active regions with a last-border endpoint at new X + 1, are
	// now active.
	for (tnI = ((m_tnY > 0) ? 0 : 1); tnI <= m_tnPGH; ++tnI)
	{
		// Get the current startpoint.
		typename BorderExtentBoundarySet::ConstIterator &ritBorder
			= m_paitBorderEndpoints[tnI];

#ifndef NDEBUG
		// Make sure it's right where we want to be.
		{
			typename BorderExtentBoundarySet::ConstIterator itPrev
				= ritBorder;
			--itPrev;
			assert (ritBorder == m_setBorderEndpoints.End()
			|| (*ritBorder).m_tnLine > m_tnY + tnI - PIXELINDEX (1)
			|| ((*ritBorder).m_tnLine == m_tnY + tnI - PIXELINDEX (1)
			|| ((*ritBorder).m_tnIndex > (m_tnX - ((tnI == 0) ? 0 : 1))
				&& (itPrev == m_setBorderEndpoints.End()
					|| (*itPrev).m_tnLine
						< m_tnY + tnI - PIXELINDEX (1)
					|| ((*itPrev).m_tnLine
						== m_tnY + tnI - PIXELINDEX (1)
						&& (*itPrev).m_tnIndex
							<= (m_tnX - ((tnI == 0) ? 0 : 1)))))));
		}
#endif // NDEBUG

		while ((--ritBorder) != m_setBorderEndpoints.End()
		&& (*ritBorder).m_tnLine == m_tnY + tnI - PIXELINDEX (1)
		&& (*ritBorder).m_tnIndex == (m_tnX - ((tnI == 0) ? 0 : 1)))
		{
			// Find the endpoint's corresponding startpoint.
			assert ((*ritBorder).m_bIsEnding);
			const BorderExtentBoundary &rHere
				= *((*ritBorder).m_pCounterpart);

#ifdef PRINT_SEARCHBORDER
			if (frame == 61)
			fprintf (stderr, "Add active-border region (%d,%d), "
					"motion vector (%d,%d)\n",
				rHere.m_tnIndex, rHere.m_tnLine,
				rHere.m_tnMotionX, rHere.m_tnMotionY);
#endif

			// Now insert it.
#ifndef NDEBUG
			typename IntersectingRegionsSet::InsertResult oInsertResult=
#endif // NDEBUG
				m_setBorderRegions.Insert (a_reStatus, rHere);
			if (a_reStatus != g_kNoError)
				return;

#ifdef PRINT_SEARCHBORDER
		if (frame == 61 && !oInsertResult.m_bInserted)
		{
			fprintf (stderr, "FOUND!\nHere's what was found:\n");
			fprintf (stderr, "\t(%d,%d), motion vector (%d,%d)\n",
				(*oInsertResult.m_itPosition).m_tnIndex,
				(*oInsertResult.m_itPosition).m_tnLine,
				(*oInsertResult.m_itPosition).m_tnMotionX,
				(*oInsertResult.m_itPosition).m_tnMotionY);
			fprintf (stderr, "Here are the active regions:\n");
			for (itRemove = m_setBorderRegions.Begin();
				 itRemove != m_setBorderRegions.End();
				 ++itRemove)
			{
				fprintf (stderr, "\t(%d,%d), motion vector (%d,%d)\n",
					(*itRemove).m_tnIndex, (*itRemove).m_tnLine,
					(*itRemove).m_tnMotionX, (*itRemove).m_tnMotionY);
			}
		}
#endif

			assert (oInsertResult.m_bInserted);
		}
		++ritBorder;
	}

	// Finally, move one step to the left.
	--m_tnX;
}



// Move down a line, finding all regions that can no longer be
// contiguous with new matches, and handing them back to the client.
template <class PIXELINDEX, class FRAMESIZE>
void
SearchBorder<PIXELINDEX,FRAMESIZE>::MoveDown (Status_t &a_reStatus)
{
	typename BorderExtentBoundarySet::Iterator itBorder, itNextBorder;
		// Used to run through the last-border.
	typename IntersectingRegionsSet::Iterator itActive, itNextActive;
		// Used to run through the active-borders set.
	int i;
		// Used to loop through things.

	// Run through the last border, disconnect the regions from all
	// endpoints.  If that leaves a region with no references and no
	// siblings, then the region is fully constructed, and it gets
	// handed back to the client.
	for (i = 0; i < 2; ++i)
	{
		BorderExtentBoundarySet &rBorder
			= ((i == 0) ? m_setBorderStartpoints: m_setBorderEndpoints);

		for (itBorder = rBorder.Begin();
			 itBorder != rBorder.End()
			 	&& (*itBorder).m_tnLine <= m_tnY - 1;
			 itBorder = itNextBorder)
		{
			// Make sure we're actually on the last-border.  (The loop
			// is more permissive than this, so that we can catch
			// errors rather than just get stuck on them.)
			assert ((*itBorder).m_tnLine == m_tnY - 1);

			// Find the next border to examine.
			itNextBorder = itBorder;
			++itNextBorder;

			// Get the endpoint here, and its under-construction region.
			BorderExtentBoundary &rEndpoint = *itBorder;
			RegionUnderConstruction *pRegion = rEndpoint.m_pRegion;

			// That's one less reference to this region.  (Note that, by
			// deliberate coincidence, setting the endpoint's region to
			// NULL won't affect the sort order, so this is safe.)
			rEndpoint.m_pRegion = NULL;
			--pRegion->m_tnReferences;
	
			// Are there any references left to this region?
			if (pRegion->m_tnReferences == 0)
			{
				// Does this region have any siblings?  (A region would
				// get siblings if it got merged with another
				// under-construction region.)
				if (pRegion->m_pForward == pRegion)
				{
					// No.  The region is fully constructed.  Move it to
					// the list of regions that'll get applied to the
					// new frame's reference-image representation.
					pRegion->Remove();	// (no more circular list)
					OnCompletedRegion (a_reStatus, pRegion->m_pRegion);
					if (a_reStatus != g_kNoError)
					{
						delete pRegion->m_pRegion;
						pRegion->m_pRegion = NULL;
						delete pRegion;
						return;
					}
				}
				else
				{
					// Yes.  Just remove ourself as a sibling.
					pRegion->Remove();
				}
	
				// We don't need this under-construction region
				// any more.
				pRegion->m_pRegion = NULL;
				delete pRegion;
			}

			// Finally, remove this startpoint/endpoint.
			rBorder.Erase (itBorder);
		}
	}

	// Run through the active-borders set, remove all references that
	// came from the now-cleared last-border.
	if (m_tnY > 0)
	{
		for (itActive = m_setBorderRegions.Begin();
			 itActive != m_setBorderRegions.End();
			 itActive = itNextActive)
		{
			// Get the next item, since we may remove the current item.
			itNextActive = itActive;
			++itNextActive;
	
			// If this region reference came from the last-border, get
			// rid of it.
			if ((*itActive).m_tnLine == m_tnY - 1)
				m_setBorderRegions.Erase (itActive);
		}
	}

	// The old last-border is gone, and we have a new current-border.
	// So move all our startpoint/endpoint iterators back one, and
	// create a new one for the new current-border.
	for (i = 1; i <= m_tnPGH; ++i)
	{
		m_paitBorderStartpoints[i-1] = m_paitBorderStartpoints[i];
		m_paitBorderEndpoints[i-1] = m_paitBorderEndpoints[i];
	}
	m_paitBorderStartpoints[m_tnPGH] = m_setBorderStartpoints.End();
	m_paitBorderEndpoints[m_tnPGH] = m_setBorderEndpoints.End();

	// Any region with (if m_tnX == 0) a last-border startpoint at
	// m_tnPGW or (if m_tnX == m_tnWidth - m_tnPGW) a last-border
	// endpoint of m_tnX will no longer be contiguous with the current
	// pixel-group, but they will be the next time the search border
	// moves left/right.
	// (But not if we're past the bottom line.  This happens when we're
	// called by FinishFrame().)
	if (m_tnY < m_tnHeight - m_tnPGH)
	{
		typename IntersectingRegionsSet::Iterator itRemove;
			// An item being removed from the possibly-intersecting set.

		if (m_tnX == 0)
		{
			// Get the iterator we'll be using.
			typename BorderExtentBoundarySet::ConstIterator &ritBorder
				= m_paitBorderStartpoints[0];
	
#ifndef NDEBUG
			// Make sure it's right where we want to be.
			{
				typename BorderExtentBoundarySet::ConstIterator itPrev
					= ritBorder;
				--itPrev;
				assert (ritBorder == m_setBorderStartpoints.End()
				|| (*ritBorder).m_tnLine > m_tnY
				|| ((*ritBorder).m_tnLine == m_tnY
					&& ((*ritBorder).m_tnIndex > m_tnPGW
						&& (itPrev == m_setBorderStartpoints.End()
							|| (*itPrev).m_tnLine < m_tnY
							|| ((*itPrev).m_tnLine == m_tnY
								&& (*itPrev).m_tnIndex <= m_tnPGW)))));
			}
#endif // NDEBUG

			// Remove all active regions that have startpoints at this
			// index.
			while ((--ritBorder) != m_setBorderStartpoints.End()
			&& (*ritBorder).m_tnLine == m_tnY
			&& (*ritBorder).m_tnIndex == m_tnPGW)
			{
#ifdef PRINT_SEARCHBORDER
				if (frame == 61)
				fprintf (stderr, "Moving down a line, remove "
						"active-border region (%d,%d), "
						"motion vector (%d,%d)\n",
					(*ritBorder).m_tnIndex, (*ritBorder).m_tnLine,
					(*ritBorder).m_tnMotionX, (*ritBorder).m_tnMotionY);
#endif
		
				itRemove = m_setBorderRegions.Find (*ritBorder);
	
#ifdef PRINT_SEARCHBORDER
				if (frame == 61 && itRemove == m_setBorderRegions.End())
				{
					fprintf (stderr, "NOT FOUND!\n"
						"Here's what was found:\n");
					for (itRemove = m_setBorderRegions.Begin();
						 itRemove != m_setBorderRegions.End();
						 ++itRemove)
					{
						fprintf (stderr, "\t(%d,%d), "
								"motion vector (%d,%d)\n",
							(*itRemove).m_tnIndex, (*itRemove).m_tnLine,
							(*itRemove).m_tnMotionX,
							(*itRemove).m_tnMotionY);
					}
				}
#endif
		
				assert (itRemove != m_setBorderRegions.End());
				m_setBorderRegions.Erase (itRemove);
			}
			++ritBorder;
		}
		else
		{
			assert (m_tnX == m_tnWidth - m_tnPGW);

			// Get the iterator we'll be using.
			typename BorderExtentBoundarySet::ConstIterator &ritBorder
				= m_paitBorderEndpoints[0];
	
#ifndef NDEBUG
			// Make sure it's right where we expect it to be.
			{
				typename BorderExtentBoundarySet::ConstIterator itPrev
					= ritBorder;
				--itPrev;
				assert (ritBorder == m_setBorderEndpoints.End()
					|| (*ritBorder).m_tnLine > m_tnY
					|| ((*ritBorder).m_tnLine == m_tnY
						&& (*ritBorder).m_tnIndex >= m_tnX
						&& (itPrev == m_setBorderEndpoints.End()
							|| (*itPrev).m_tnLine < m_tnY
							|| ((*itPrev).m_tnLine == m_tnY
								&& (*itPrev).m_tnIndex < m_tnX))));
			}
#endif // NDEBUG
	
			// Remove all active regions that have endpoints at this
			// index.
			while (ritBorder != m_setBorderEndpoints.End()
			&& (*ritBorder).m_tnLine == m_tnY
			&& (*ritBorder).m_tnIndex == m_tnX)
			{
				// Move to the corresponding startpoint.
				const BorderExtentBoundary &rHere
					= *((*ritBorder).m_pCounterpart);

#ifdef PRINT_SEARCHBORDER
				if (frame == 61)
				fprintf (stderr, "Moving down a line, remove "
						"active-border region (%d,%d), "
						"motion vector (%d,%d)\n",
					rHere.m_tnIndex, rHere.m_tnLine,
					rHere.m_tnMotionX, rHere.m_tnMotionY);
#endif
		
				itRemove = m_setBorderRegions.Find (rHere);
	
#ifdef PRINT_SEARCHBORDER
				if (frame == 61 && itRemove == m_setBorderRegions.End())
				{
					fprintf (stderr, "NOT FOUND!\n"
						"Here's what was found:\n");
					for (itRemove = m_setBorderRegions.Begin();
						 itRemove != m_setBorderRegions.End();
						 ++itRemove)
					{
						fprintf (stderr, "\t(%d,%d), "
								"motion vector (%d,%d)\n",
							(*itRemove).m_tnIndex, (*itRemove).m_tnLine,
							(*itRemove).m_tnMotionX,
							(*itRemove).m_tnMotionY);
					}
				}
#endif
		
				assert (itRemove != m_setBorderRegions.End());
				m_setBorderRegions.Erase (itRemove);

				++ritBorder;
			}
		}

		// Finally, move one step down, and prepare to move the other
		// direction across the window.  (But not if we're past the
		// bottom line, i.e. being called by FinishFrame().)
		++m_tnY;
		m_tnStepX = -m_tnStepX;
	}
}



// Return the number of regions that would intersect with the
// current pixel-group.
template <class PIXELINDEX, class FRAMESIZE>
inline FRAMESIZE
SearchBorder<PIXELINDEX,FRAMESIZE>::NumberOfActiveRegions (void) const
{
	// Easy enough.
	return m_setBorderRegions.Size();
}



// Accept a new match for a pixel-group, with the given motion-vector.
template <class PIXELINDEX, class FRAMESIZE>
FRAMESIZE
SearchBorder<PIXELINDEX,FRAMESIZE>::AddNewMatch (Status_t &a_reStatus,
	PIXELINDEX a_tnMotionX, PIXELINDEX a_tnMotionY)
{
	PIXELINDEX tnI, tnJ;
		// Used to loop through things.
	typename IntersectingRegionsSet::Iterator itHere;
		// The range of regions that are contiguous with the newly-added
		// pixel-group.
	bool bLastBorderExtentFound;
		// true if the current pixel-group merged with a region that
		// had an extent on the last-boundary.
	MovedRegion *pRegionMergedTo;
	RegionUnderConstruction *pRegionUnderConstructionMergedTo;
		// The region that receives the new pixel-group, as well as any
		// other region that's now contiguous with it.
		// May be a brand-new region.
	BorderExtentBoundary *aStart, *aEnd;
		// Used to search for the range of regions contiguous with the
		// new pixel-group, as well as to set up the new startpoints and
		// endpoints for the current-border and last-border.
	typename BorderExtentBoundarySet::Iterator *aitCurrentStarts,
			*aitCurrentEnds;
		// Where the startpoints/endpoints gets hooked into the
		// current-boundary/last-boundary.
	typename IntersectingRegionsSet::Iterator *aitCurrentRegions;
		// Where the new/merged region gets hooked into the
		// possibly-intersecting list of regions.
	
	// Allocate memory for our temporary arrays.
	aStart = (BorderExtentBoundary *) alloca
		(ARRAYSIZE (BorderExtentBoundary, m_tnPGH + 1));
	aEnd = (BorderExtentBoundary *) alloca
		(ARRAYSIZE (BorderExtentBoundary, m_tnPGH + 1));
	aitCurrentStarts = (typename BorderExtentBoundarySet::Iterator *)
		alloca (ARRAYSIZE (typename BorderExtentBoundarySet::Iterator,
		m_tnPGH + 1));
	aitCurrentEnds = (typename BorderExtentBoundarySet::Iterator *)
		alloca (ARRAYSIZE (typename BorderExtentBoundarySet::Iterator,
		m_tnPGH + 1));
	aitCurrentRegions = (typename IntersectingRegionsSet::Iterator *)
		alloca (ARRAYSIZE (typename IntersectingRegionsSet::Iterator,
		m_tnPGH + 1));

#if 0
	// (Why isn't this working???  It's not strictly necessary, but I
	// like to be clean.)
	// Construct the objects in our temporary arrays.
	for (tnI = 0; tnI < m_tnPGH + 1; ++tnI)
	{
		typedef typename BorderExtentBoundarySet::Iterator
			BorderExtentBoundarySetIterator;
		typedef typename IntersectingRegionsSet::Iterator
			IntersectingRegionsSetIterator;

		aStart[tnI].BorderExtentBoundary();
		aEnd[tnI].BorderExtentBoundary();
		aitCurrentStarts[tnI].BorderExtentBoundarySetIterator();
		aitCurrentEnds[tnI].BorderExtentBoundarySetIterator();
		aitCurrentRegions[tnI].IntersectingRegionsSetIterator();
	}
#endif

	// m_setBorderRegions contains all the regions that would intersect
	// with a pixel-group match at the current location, so we just
	// search for the subset with the same motion-vector.
	aStart[0].m_tnMotionX = a_tnMotionX;
	aStart[0].m_tnMotionY = a_tnMotionY;
	aStart[0].m_bIsEnding = false;
	itHere = m_setBorderRegions.LowerBound (aStart[0]);

	// Create the initial startpoint/endpoint for the current-border
	// and last-border.  If we find any regions that intersect with the
	// new pixel-group, these will get adjusted.
	for (tnI = 0; tnI <= m_tnPGH; ++tnI)
	{
		aStart[tnI].m_tnMotionX = aEnd[tnI].m_tnMotionX = a_tnMotionX;
		aStart[tnI].m_tnMotionY = aEnd[tnI].m_tnMotionY = a_tnMotionY;
		aStart[tnI].m_bIsEnding = false;
		aEnd[tnI].m_bIsEnding = true;
		aStart[tnI].m_tnIndex = m_tnX;
		aEnd[tnI].m_tnIndex = m_tnX + m_tnPGW;
		aStart[tnI].m_tnLine = aEnd[tnI].m_tnLine = m_tnY + tnI
			- PIXELINDEX (1);
	}

	// We only add a last-border extent if the current pixel-group
	// got merged with a region that had a last-border extent (since the
	// current pixel-group doesn't have a last-border extent itself).
	// Set up to detect that.
	bLastBorderExtentFound = false;
	aStart[0].m_tnIndex = Limits<PIXELINDEX>::Max;
	aEnd[0].m_tnIndex = Limits<PIXELINDEX>::Min;

	// Do any existing regions intersect with the new pixel-group?
	if (itHere == m_setBorderRegions.End()
	|| (*itHere).m_tnMotionX != aStart[0].m_tnMotionX
	|| (*itHere).m_tnMotionY != aStart[0].m_tnMotionY)
	{
#ifdef PRINT_SEARCHBORDER
		if (frame == 61)
		fprintf (stderr, "Create a new region for the match.\n");
#endif

		// No.  Create a new region.
		pRegionMergedTo = new MovedRegion (a_reStatus);
		if (pRegionMergedTo == NULL)
			goto cleanup0;
		if (a_reStatus != g_kNoError)
			goto cleanup1;

		// Set its motion vector.
		pRegionMergedTo->SetMotionVector (a_tnMotionX, a_tnMotionY);

		// Create something to manage the region as it's being
		// constructed.
		pRegionUnderConstructionMergedTo = new RegionUnderConstruction;
		if (pRegionUnderConstructionMergedTo == NULL)
			goto cleanup1;
		pRegionUnderConstructionMergedTo->m_pRegion = pRegionMergedTo;

		// Regions under construction form a circular list with all
		// other under-construction regions that it gets merged with.
		// So start the circular list.
		pRegionUnderConstructionMergedTo->m_pForward
			= pRegionUnderConstructionMergedTo->m_pBackward
			= pRegionUnderConstructionMergedTo;
	}

	// Some existing regions intersect with the current pixel-group.
	else
	{
#ifdef PRINT_SEARCHBORDER
		if (frame == 61)
		fprintf (stderr, "This match is contiguous with an existing "
			"region!\n");
#endif

		// Get the first region that's contiguous with the new
		// pixel-group.
		pRegionUnderConstructionMergedTo = (*itHere).m_pRegion;
		pRegionMergedTo = pRegionUnderConstructionMergedTo->m_pRegion;

#ifdef PRINT_SEARCHBORDER
		if (frame == 61)
		{
		fprintf (stderr, "Region to merge to:\n");
		PrintRegion (*pRegionMergedTo);
		fprintf (stderr, "\n");
		}
#endif

		// Now loop through all the regions that'll get merged with the
		// current pixel-group (including the first one that we already
		// looked at).  Incorporate the startpoint/endpoint associated
		// with that region into our new startpoint/endpoint.  Then, if
		// it's a different region than the first one we looked at,
		// merge it into that first one.  Finally, remove the found
		// startpoints/endpoints -- they'll be superseded by the
		// merged-region's startpoints/endpoints.
		while (itHere != m_setBorderRegions.End()
		&& (*itHere).m_tnMotionX == aStart[0].m_tnMotionX
		&& (*itHere).m_tnMotionY == aStart[0].m_tnMotionY)
		{
			typename IntersectingRegionsSet::Iterator itNext;
				// The next item in the possibly-intersecting-region
				// set.  Needed because we remove the current item.
			PIXELINDEX tnBorderIndex;
				// Which current-border it's on.  Ranges from 0 (the
				// last-border) to 1 (the first current-border) to
				// m_tnPGH (the last current-border).
			MovedRegion *pRegionMergedFrom;
			RegionUnderConstruction *pRegionUnderConstructionMergedFrom;
				// The latest region that's contiguous with the new
				// pixel-group.
			typename BorderExtentBoundarySet::Iterator itStart, itEnd;
				// The location of the startpoint/endpoint of the latest
				// region that's contiguous with the new pixel-group.
				// (And they all lived in the house that Jack built, so
				// NYAAAH!)

			// Before we have a chance to remove the current item,
			// remember the next item, so that we can move to it later.
			itNext = itHere;
			++itNext;

			// Figure out which border this new region is on.
			tnBorderIndex = (*itHere).m_tnLine - m_tnY + 1;
			// (Sanity check: there should be no last-border regions
			// if there is no last-border.)
			assert (tnBorderIndex > 0 || m_tnY > 0);

#ifdef PRINT_SEARCHBORDER
			if (frame == 61)
			fprintf (stderr, "Search for startpoint (%d,%d)\n",
				(*itHere).m_tnIndex, (*itHere).m_tnLine);
#endif

			// Find the startpoint/endpoint.
			itStart = m_setBorderStartpoints.Find (*itHere);
			assert (itStart != m_setBorderStartpoints.End());
			assert (*itStart == *itHere);
			itEnd = m_setBorderEndpoints.Find
				(*((*itStart).m_pCounterpart));
			assert (itEnd != m_setBorderEndpoints.End());
			// (Sanity check: make sure the endpoint knows it's paired
			// with this startpoint.)
			assert ((*itEnd).m_pCounterpart == &(*itStart));
			// (Sanity check: make sure they refer to the same region
			// under construction.)
			assert ((*itStart).m_pRegion == (*itEnd).m_pRegion);

			// Remove the merged-from region from the list of
			// possibly-intersecting regions.
			assert (m_setBorderRegions.LowerBound (*itHere)	// sanity
				== itHere);
			m_setBorderRegions.Erase (itHere);

			// Extend our new startpoint/endpoint by this region's
			// startpoint/endpoint.
			aStart[tnBorderIndex].m_tnIndex
				= Min (aStart[tnBorderIndex].m_tnIndex,
					(*itStart).m_tnIndex);
			aEnd[tnBorderIndex].m_tnIndex
				= Max (aEnd[tnBorderIndex].m_tnIndex,
					(*itEnd).m_tnIndex);
			// (If that was on the last-border, remember that.)
			if (tnBorderIndex == 0)
				bLastBorderExtentFound = true;

			// Get the region that'll be merged from.
			pRegionUnderConstructionMergedFrom = (*itStart).m_pRegion;
			pRegionMergedFrom = pRegionUnderConstructionMergedFrom
				->m_pRegion;

			// If this region is different than the first region we
			// looked at, merge it with that first region.
			if (pRegionMergedFrom != pRegionMergedTo)
			{
				RegionUnderConstruction *pCurrentRUC, *pNextRUC;
					// The regions under construction that we move to
					// the merged-to region's circular list.

#ifdef PRINT_SEARCHBORDER
				if (frame == 61)
				{
				fprintf (stderr, "Region to merge from:\n");
				PrintRegion (*pRegionMergedFrom);
				fprintf (stderr, "\n");
				}
#endif

				// Merge the regions together.  (Merge() only works on
				// regions that don't already intersect, for speed
				// reasons; the way we manage the border ensures that.
				// How handy for us.)
				pRegionMergedTo->Merge (*pRegionMergedFrom);

				// Run through the circular list of regions under
				// construction, the ones pointing to the merged-from
				// region, update each one to point to the merged-to
				// region, and put each one into the merged-to region's
				// circular list.
				pCurrentRUC = pRegionUnderConstructionMergedFrom;
				for (;;)
				{
					// Remember the next region in the circular list.
					pNextRUC = pCurrentRUC->m_pForward;

					// (Sanity check: make sure everyone in this
					// circular list points to the same region.)
					assert (pCurrentRUC->m_pRegion
						== pRegionMergedFrom);

					// Update the region it refers to.
					pCurrentRUC->m_pRegion = pRegionMergedTo;

					// Move it into the merged-to region's circular
					// list.
					pCurrentRUC->Remove();
					pRegionUnderConstructionMergedTo->InsertBefore
						(pRegionUnderConstructionMergedTo, pCurrentRUC);

					// If all regions have been removed from this
					// circular list, stop.
					if (pCurrentRUC == pNextRUC)
						break;

					// Move to the next region in the circular list.
					pCurrentRUC = pNextRUC;
				}
			}

			// Finally, remove the startpoint/endpoint we found for this
			// region, adjusting reference counts & cleaning up as
			// necessary.
			assert (m_setBorderEndpoints.LowerBound (*itEnd)
				== itEnd);	// sanity
			for (tnJ = 0; tnJ <= m_tnPGH; ++tnJ)
				if (m_paitBorderEndpoints[tnJ] == itEnd)
					++m_paitBorderEndpoints[tnJ];
			m_setBorderEndpoints.Erase (itEnd);
			assert (m_setBorderStartpoints.LowerBound (*itStart)
				== itStart);	// sanity
			for (tnJ = 0; tnJ <= m_tnPGH; ++tnJ)
				if (m_paitBorderStartpoints[tnJ] == itStart)
					++m_paitBorderStartpoints[tnJ];
			m_setBorderStartpoints.Erase (itStart);
			pRegionUnderConstructionMergedFrom->m_tnReferences -= 2;
			if (pRegionUnderConstructionMergedFrom
				!= pRegionUnderConstructionMergedTo
			&& pRegionUnderConstructionMergedFrom->m_tnReferences == 0)
			{
				// This region under construction, now pointing to the
				// merged-to region, has no more references to it.  Get
				// rid of it.
				assert (pRegionUnderConstructionMergedFrom->m_pRegion
					== pRegionMergedTo);
				assert (pRegionUnderConstructionMergedFrom->m_pForward
					!= pRegionUnderConstructionMergedFrom);
				pRegionUnderConstructionMergedFrom->Remove();
				pRegionUnderConstructionMergedFrom->m_pRegion = NULL;
			 	delete pRegionUnderConstructionMergedFrom;
			}

			// We're done with this region.
			if (pRegionMergedFrom != pRegionMergedTo)
				delete pRegionMergedFrom;

			// Move to the next item in the set.
			itHere = itNext;
		}
	}

	// At this point, the region that contains the new pixel-group has
	// either been created, or it's been retrieved from the
	// possibly-intersecting set, and all regions that are contiguous
	// with the new pixel-group have been merged into it.  But the new
	// pixel-group hasn't been added yet in either case.

	// Add the pixel-group's extents to this new/merged region.
	{
		PIXELINDEX tnY;
			// Used to loop through the pixel-group's lines.

#ifdef PRINTREGIONMATH
		bool bRegionEmpty = ((pRegionMergedTo->NumberOfPoints() == 0)
			? true : false);
#endif // PRINTREGIONMATH

		// Add each line in the pixel-group.
		for (tnY = m_tnY; tnY < m_tnY + m_tnPGH; ++tnY)
		{
#ifdef PRINTREGIONMATH
			//if (!bRegionEmpty /* && frame == 4 */)
			{
				fprintf (stderr, "Before Union with [%d, %d-%d]:\n",
					int (tnY), int (m_tnX), int (m_tnX + m_tnPGW));
				PrintRegion (*pRegionMergedTo);
				fprintf (stderr, "\n");
			}
#endif // PRINTREGIONMATH

			pRegionMergedTo->Union (a_reStatus, tnY, m_tnX,
				m_tnX + m_tnPGW);
			if (a_reStatus != g_kNoError)
			{
				// We couldn't add the new pixel-group.  If this
				// region has no references, we have to delete it
				// now.  (It could have no references because it's
				// newly created, or because all references to it
				// were removed from the search-border.)
				if (pRegionUnderConstructionMergedTo->m_tnReferences
					== 0)
						goto cleanup2;

				// Return with our error.
				return 0;
			}

#ifdef PRINTREGIONMATH
			//if (!bRegionEmpty /* && frame == 4 */)
			{
				fprintf (stderr, "After Union with [%d, %d-%d]:\n",
					int (tnY), int (m_tnX), int (m_tnX + m_tnPGW));
				PrintRegion (*pRegionMergedTo);
				fprintf (stderr, "\n");
				getchar();
				fprintf (stderr, "\n");
			}
#endif // PRINTREGIONMATH
		}
	}

#ifdef PRINT_SEARCHBORDER
	if (frame == 61)
	{
	fprintf (stderr, "Final new/merged region:\n");
	PrintRegion (*pRegionMergedTo);
	fprintf (stderr, "\n");
	}
#endif

	// Hook the new/merged region to our startpoints/endpoints.
	for (tnI = 0; tnI <= m_tnPGH; ++tnI)
		aStart[tnI].m_pRegion = aEnd[tnI].m_pRegion
			= pRegionUnderConstructionMergedTo;

	// Add all startpoint/endpoints to the current-boundary lines.
	for (tnI = 0; tnI <= m_tnPGH; ++tnI)
	{
		// (If there is no last-border extent, skip adding the
		// last-border startpoint/endpoint.)
		if (tnI == 0 && !bLastBorderExtentFound)
			continue;

		// Add this startpoint to the current-boundary.
		{
			typename BorderExtentBoundarySet::InsertResult oInsertResult
				= m_setBorderStartpoints.Insert (a_reStatus,
					aStart[tnI]);
			if (a_reStatus != g_kNoError)
				goto cleanup5;
			assert (oInsertResult.m_bInserted);
	
			// Remember where it got inserted.
			aitCurrentStarts[tnI] = oInsertResult.m_itPosition;
	
			// That's one more reference to this region under
			// construction.
			++pRegionUnderConstructionMergedTo->m_tnReferences;

			// If this new item was inserted before the current item on
			// this line, and it's got an equivalent index value, it
			// becomes the new current item.
			if (m_tnStepX == 1)
			{
				++oInsertResult.m_itPosition;
				for (tnJ = 0; tnJ <= m_tnPGH; ++tnJ)
				{
					if (m_paitBorderStartpoints[tnJ]
						== oInsertResult.m_itPosition
					&& ((*aitCurrentStarts[tnI]).m_tnLine
						> m_tnY + tnJ - PIXELINDEX (1)
					|| ((*aitCurrentStarts[tnI]).m_tnLine
							== m_tnY + tnJ - PIXELINDEX (1)
						&& (*aitCurrentStarts[tnI]).m_tnIndex
							>= (m_tnX + m_tnPGW + 1 -
								((tnJ == 0) ? 1 : 0)))))
					{
						--m_paitBorderStartpoints[tnJ];
						assert (m_paitBorderStartpoints[tnJ]
							== aitCurrentStarts[tnI]);
					}
				}
			}
			else
			{
				assert (m_tnStepX == -1);
				++oInsertResult.m_itPosition;
				for (tnJ = 0; tnJ <= m_tnPGH; ++tnJ)
				{
					if (m_paitBorderStartpoints[tnJ]
						== oInsertResult.m_itPosition
					&& ((*aitCurrentStarts[tnI]).m_tnLine
						> m_tnY + tnJ - PIXELINDEX (1)
					|| ((*aitCurrentStarts[tnI]).m_tnLine
							== m_tnY + tnJ - PIXELINDEX (1)
						&& (*aitCurrentStarts[tnI]).m_tnIndex
							> (m_tnX + m_tnPGW
								- ((tnJ == 0) ? 1 : 0)))))
					{
						--m_paitBorderStartpoints[tnJ];
						assert (m_paitBorderStartpoints[tnJ]
							== aitCurrentStarts[tnI]);
					}
				}
			}
		}
	
		// Add this endpoint to the current-boundary.
		{
			typename BorderExtentBoundarySet::InsertResult oInsertResult
				= m_setBorderEndpoints.Insert (a_reStatus, aEnd[tnI]);
			if (a_reStatus != g_kNoError)
				goto cleanup3;
			assert (oInsertResult.m_bInserted);
	
			// Remember where it got inserted.
			aitCurrentEnds[tnI] = oInsertResult.m_itPosition;
	
			// That's one more reference to this region under
			// construction.
			++pRegionUnderConstructionMergedTo->m_tnReferences;

			// If this new item was inserted before the current item on
			// this line, and it's got an equivalent index value, it
			// becomes the new current item.
			if (m_tnStepX == 1)
			{
				++oInsertResult.m_itPosition;
				for (tnJ = 0; tnJ <= m_tnPGH; ++tnJ)
				{
					if (m_paitBorderEndpoints[tnJ]
						== oInsertResult.m_itPosition
					&& ((*aitCurrentEnds[tnI]).m_tnLine
						> m_tnY + tnJ - PIXELINDEX (1)
					|| ((*aitCurrentEnds[tnI]).m_tnLine
							== m_tnY + tnJ - PIXELINDEX (1)
						&& (*aitCurrentEnds[tnI]).m_tnIndex
							>= (m_tnX + ((tnJ == 0) ? 1 : 0)))))
					{
						--m_paitBorderEndpoints[tnJ];
						assert (m_paitBorderEndpoints[tnJ]
							== aitCurrentEnds[tnI]);
					}
				}
			}
			else
			{
				++oInsertResult.m_itPosition;
				for (tnJ = 0; tnJ <= m_tnPGH; ++tnJ)
				{
					if (m_paitBorderEndpoints[tnJ]
						== oInsertResult.m_itPosition
					&& ((*aitCurrentEnds[tnI]).m_tnLine
						> m_tnY + tnJ - PIXELINDEX (1)
					|| ((*aitCurrentEnds[tnI]).m_tnLine
							== m_tnY + tnJ - PIXELINDEX (1)
						&& (*aitCurrentEnds[tnI]).m_tnIndex
							> (m_tnX - 1 + ((tnJ == 0) ? 1 : 0)))))
					{
						--m_paitBorderEndpoints[tnJ];
						assert (m_paitBorderEndpoints[tnJ]
							== aitCurrentEnds[tnI]);
					}
				}
			}
		}
	
		// Now that startpoint & endpoint are inserted, we can set up
		// their counterpart links.
		(*aitCurrentStarts[tnI]).m_pCounterpart
			= &(*aitCurrentEnds[tnI]);
		(*aitCurrentEnds[tnI]).m_pCounterpart
			= &(*aitCurrentStarts[tnI]);
	
		// Add this region under construction to the list of regions
		// that intersect with the current pixel-group.  (Since this
		// new region contains the current pixel-group, obviously it
		// intersects.)
		{
			typename IntersectingRegionsSet::InsertResult oInsertResult
				= m_setBorderRegions.Insert (a_reStatus,
					*(aitCurrentStarts[tnI]));
			if (a_reStatus != g_kNoError)
				goto cleanup4;
			assert (oInsertResult.m_bInserted);
	
			// Remember where it got inserted, in case the operation
			// fails and we have to clean up after ourselves.
			aitCurrentRegions[tnI] = oInsertResult.m_itPosition;
	
#ifdef PRINT_SEARCHBORDER
			if (frame == 61)
			{
			fprintf (stderr, "Inserted border startpoint "
				"(%d,%d), motion vector (%d,%d)\n",
				(*aitCurrentRegions[tnI]).m_tnIndex,
				(*aitCurrentRegions[tnI]).m_tnLine,
				(*aitCurrentRegions[tnI]).m_tnMotionX,
				(*aitCurrentRegions[tnI]).m_tnMotionY);
			fprintf (stderr, "\tcorresponding endpoint at (%d,%d)\n",
				(*aitCurrentEnds[tnI]).m_tnIndex,
				(*aitCurrentEnds[tnI]).m_tnLine);
			}
#endif
		}
	}

	// If this is the first added match, then the last-border startpoint
	// and endpoint start off as one past the last item on its line,
	// i.e. the first item on the next line, i.e. the first item in the
	// set.
	if (m_paitBorderStartpoints[0] == m_setBorderStartpoints.End()
	&& m_paitBorderEndpoints[0] == m_setBorderEndpoints.End())
	{
#ifdef PRINT_SEARCHBORDER
		if (frame == 61)
		fprintf (stderr, "Initialized last-border "
			"startpoint/endpoint.\n");
#endif // PRINT_SEARCHBORDER
		m_paitBorderStartpoints[0] = m_setBorderStartpoints.Begin();
		m_paitBorderEndpoints[0] = m_setBorderEndpoints.Begin();
	}

	// All done.
	return pRegionMergedTo->NumberOfPoints();

	// Handle errors.
cleanup5:
	for (;;)
	{
		m_setBorderRegions.Erase (aitCurrentRegions[tnI]);
cleanup4:
		for (tnJ = 0; tnJ <= m_tnPGH; ++tnJ)
			if (m_paitBorderEndpoints[tnJ] == aitCurrentEnds[tnI])
				++m_paitBorderEndpoints[tnJ];
		m_setBorderEndpoints.Erase (aitCurrentEnds[tnI]);
		--pRegionUnderConstructionMergedTo->m_tnReferences;
cleanup3:
		for (tnJ = 0; tnJ <= m_tnPGH; ++tnJ)
			if (m_paitBorderStartpoints[tnJ] == aitCurrentStarts[tnI])
				++m_paitBorderStartpoints[tnJ];
		m_setBorderStartpoints.Erase (aitCurrentStarts[tnI]);
		--pRegionUnderConstructionMergedTo->m_tnReferences;
		if (tnI == 0)
			break;
		--tnI;
		if (m_tnY == 0 && tnI == 0)		// skip nonexistent last-borders
			break;
	}
cleanup2:
	if (pRegionUnderConstructionMergedTo->m_tnReferences == 0)
	{
		if (pRegionUnderConstructionMergedTo->m_pForward
			== pRegionUnderConstructionMergedTo)
				delete pRegionMergedTo;
		pRegionUnderConstructionMergedTo->Remove();
		pRegionUnderConstructionMergedTo->m_pRegion = NULL;
		delete pRegionUnderConstructionMergedTo;
	}
	return 0;
cleanup1:
	delete pRegionMergedTo;
cleanup0:
	return 0;
}



// Remove all regions that matched the current pixel-group, except for
// the single best one, and return it.
template <class PIXELINDEX, class FRAMESIZE>
typename SearchBorder<PIXELINDEX,FRAMESIZE>::MovedRegion *
SearchBorder<PIXELINDEX,FRAMESIZE>::ChooseBestActiveRegion
	(Status_t &a_reStatus)
{
	RegionUnderConstruction *pUnderConstructionSurvivor;
	MovedRegion *pSurvivor;
		// The only region to survive pruning -- the biggest one, with
		// the shortest motion vector.
	typename IntersectingRegionsSet::Iterator itHere;
		// The range of regions that are contiguous with the newly-added
		// pixel-group.
	PIXELINDEX tnI;
		// Used to loop through iterators.
	typename BorderExtentBoundarySet::Iterator itStart, itEnd,
			itNextStart;
		// Startpoints/endpoints that refer to the flood-filled region.

	// Make sure they didn't start us off with an error.
	assert (a_reStatus == g_kNoError);

	// m_setBorderRegions contains all the regions that intersected
	// with the current pixel-group.  Find the largest region, and,
	// for equal-sized regions, the one with the shortest motion
	// vector.
	pUnderConstructionSurvivor = NULL;
	for (itHere = m_setBorderRegions.Begin();
		 itHere != m_setBorderRegions.End();
		 ++itHere)
	{
		if (pUnderConstructionSurvivor == NULL)
			pUnderConstructionSurvivor = (*itHere).m_pRegion;
		else if ((*itHere).m_pRegion->m_pRegion
				== pUnderConstructionSurvivor->m_pRegion)
			/* Duplicate */;
		else if (pUnderConstructionSurvivor->m_pRegion->NumberOfPoints()
				> (*itHere).m_pRegion->m_pRegion->NumberOfPoints())
			/* Current survivor has more points */;
		else if (pUnderConstructionSurvivor->m_pRegion->NumberOfPoints()
				< (*itHere).m_pRegion->m_pRegion->NumberOfPoints())
			pUnderConstructionSurvivor = (*itHere).m_pRegion;
		else if (pUnderConstructionSurvivor->m_pRegion
				->GetSquaredMotionVectorLength()
			> (*itHere).m_pRegion->m_pRegion
				->GetSquaredMotionVectorLength())
			pUnderConstructionSurvivor = (*itHere).m_pRegion;
	}

	// Get the one surviving region.
	pSurvivor = pUnderConstructionSurvivor->m_pRegion;

	// Now run through the regions that intersected the current
	// pixel-group, and change all of them to point to the survivor.
	// This serves to clean up all references to the regions, and
	// to mark all of them for later removal.
	for (itHere = m_setBorderRegions.Begin();
		 itHere != m_setBorderRegions.End();
		 ++itHere)
	{
		RegionUnderConstruction *pPrunedRUC, *pNextPrunedRUC;
		MovedRegion *pPrunedRegion;
			// The region getting pruned.

		// Get the current region.
		pPrunedRUC = (*itHere).m_pRegion;
		pPrunedRegion = pPrunedRUC->m_pRegion;

		// If this region already points to the survivor, it doesn't
		// need to be changed.
		if (pPrunedRegion == pSurvivor)
			continue;

		// Run through all the siblings, change them to point to the
		// survivor.
		for (;;)
		{
			// Remember the next sibling.
			pNextPrunedRUC = pPrunedRUC->m_pForward;

			// Sanity check.
			assert (pPrunedRUC->m_pRegion == pPrunedRegion);

			// Remove this sibling from its current ring.
			pPrunedRUC->Remove();

			// Point it to the survivor.
			pPrunedRUC->m_pRegion = pSurvivor;

			// Make this a sibling of the survivor.
			pUnderConstructionSurvivor->InsertBefore
				(pUnderConstructionSurvivor, pPrunedRUC);

			// If we're out of siblings, stop.
			if (pPrunedRUC == pNextPrunedRUC)
				break;

			// Move to the next sibling.
			pPrunedRUC = pNextPrunedRUC;
		}

		// All references to this region have been cleaned up.  Get
		// rid of it.
		delete pPrunedRegion;
	}

	// Now there are no more regions on the border.
	m_setBorderRegions.Clear();

	// Now run through all startpoints/endpoints, find the ones that
	// refer to the survivor region, and get rid of them.
	for (itStart = m_setBorderStartpoints.Begin();
		 itStart != m_setBorderStartpoints.End();
		 itStart = itNextStart)
	{
		RegionUnderConstruction *pPrunedRegion;
			// The region getting pruned.
		PIXELINDEX tnBorderIndex;
			// Which border this startpoint is on.

		// Remember the next startpoint.
		itNextStart = itStart;
		++itNextStart;

		// Get the region to prune.
		pPrunedRegion = (*itStart).m_pRegion;

		// If this startpoint doesn't refer to the survivor,
		// move on.
		if (pPrunedRegion->m_pRegion != pSurvivor)
			continue;

		// Figure out which border it's on.
		tnBorderIndex = (*itStart).m_tnLine - m_tnY;

		// Find the corresponding endpoint.
		itEnd = m_setBorderEndpoints.Find
			(*((*itStart).m_pCounterpart));
		assert (itEnd != m_setBorderEndpoints.End());
		// (Sanity check: make sure the endpoint knows it's paired
		// with this startpoint.)
		assert ((*itEnd).m_pCounterpart == &(*itStart));
		// (Sanity check: make sure they refer to the same region
		// under construction.)
		assert ((*itStart).m_pRegion == (*itEnd).m_pRegion);

		// Remove the startpoint/endpoint, adjusting reference
		// counts & cleaning up as necessary.
		assert (m_setBorderEndpoints.LowerBound (*itEnd)
			== itEnd);	// sanity
		for (tnI = 0; tnI <= m_tnPGH; ++tnI)
			if (m_paitBorderEndpoints[tnI] == itEnd)
				++m_paitBorderEndpoints[tnI];
		m_setBorderEndpoints.Erase (itEnd);
		assert (m_setBorderStartpoints.LowerBound (*itStart)
			== itStart);	// sanity
		for (tnI = 0; tnI <= m_tnPGH; ++tnI)
			if (m_paitBorderStartpoints[tnI] == itStart)
				++m_paitBorderStartpoints[tnI];
		m_setBorderStartpoints.Erase (itStart);
		pPrunedRegion->m_tnReferences -= 2;
		if (pPrunedRegion->m_tnReferences == 0)
		{
			// This region under construction has no more references
			// to it.  Get rid of it.
			pPrunedRegion->Remove();
			pPrunedRegion->m_pRegion = NULL;
		 	delete pPrunedRegion;
		}
	}

	// Return the region.
	return pSurvivor;
}



// Clean up the search border at the end of a frame, e.g. hand all
// remaining regions back to the client.
template <class PIXELINDEX, class FRAMESIZE>
void
SearchBorder<PIXELINDEX,FRAMESIZE>::FinishFrame (Status_t &a_reStatus)
{
	// Make sure they didn't start us off with an error.
	assert (a_reStatus == g_kNoError);

	// It turns out that we can accomplish this by pretending to move
	// down m_tnPGH+1 lines -- that'll clear out the last-border and the
	// current-border.
	for (int i = 0; i <= m_tnPGH; ++i)
	{
		MoveDown (a_reStatus);
		if (a_reStatus != g_kNoError)
			return;
		++m_tnY;
	}

	// Make sure that emptied the border.
	assert (m_setBorderStartpoints.Size() == 0);
	assert (m_setBorderEndpoints.Size() == 0);
	assert (m_setBorderRegions.Size() == 0);
}



#endif // __SEARCH_BORDER_H__
