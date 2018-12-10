/*
 * SFC_Tree.cpp
 *
 * Masado Ishii  --  UofU SoC, 2018-12-03
 *
 * Based on work by Milinda Fernando and Hari Sundar.
 *   - Algorithms: SC18 "Comparison Free Computations..." TreeSort, TreeConstruction, TreeBalancing
 *   - Code: Dendro4 [sfcSort.h] [construct.cpp]
 *
 * My contribution is to extend the data structures to 4 dimensions (or higher).
 */

#include "SFC_Tree.h"
#include "hcurvedata.h"
#include "PROXY_parUtils.h"

#include <stdio.h>

namespace ot
{


//
// locTreeSort()
//
template<typename T, unsigned int D>
void
SFC_Tree<T,D>:: locTreeSort(TreeNode<T,D> *points,
                          RankI begin, RankI end,
                          LevI sLev,
                          LevI eLev,
                          RotI pRot,
                          std::vector<BucketInfo<RankI>> &outBuckets,  //TODO remove
                          bool makeBuckets)                                   //TODO remove
{
  //// Recursive Depth-first, similar to Most Significant Digit First. ////

  if (end <= begin) { return; }

  constexpr char numChildren = TreeNode<T,D>::numChildren;
  constexpr unsigned int rotOffset = 2*numChildren;  // num columns in rotations[].

  // Reorder the buckets on sLev (current level).
  std::array<RankI, numChildren+1> tempSplitters;
  SFC_bucketing(points, begin, end, sLev, pRot, tempSplitters);
  // The array `tempSplitters' has numChildren+1 slots, which includes the
  // beginning, middles, and end of the range of children.

  // Lookup tables to apply rotations.
  const ChildI * const rot_perm = &rotations[pRot*rotOffset + 0*numChildren];
  const RotI * const orientLookup = &HILBERT_TABLE[pRot*numChildren];

  if (sLev < eLev)  // This means eLev is further from the root level than sLev.
  {
    const unsigned int continueThresh = makeBuckets ? 0 : 1;
    //TODO can remove the threshold. We only need the splitters to recurse.

    // Recurse.
    // Use the splitters to specify ranges for the next level of recursion.
    // Use the results of the recursion to build the list of ending buckets.  //TODO no buckets
    // While at first it might seem wasteful that we keep sorting the
    // ancestors and they don't move, in reality there are few ancestors,
    // so it (probably) doesn't matter that much.
    for (char child_sfc = 0; child_sfc < numChildren; child_sfc++)
    {
      // Columns of HILBERT_TABLE are indexed by the Morton rank.
      // According to Dendro4 TreeNode.tcc:199 they are.
      // (There are possibly inconsistencies in the old code...?
      // Don't worry, we can regenerate the table later.)
      ChildI child = rot_perm[child_sfc] - '0';     // Decode from human-readable ASCII.
      RotI cRot = orientLookup[child];

      if (tempSplitters[child_sfc+1] - tempSplitters[child_sfc] <= continueThresh)
        continue;
      // We don't skip a singleton, since a singleton contributes a bucket.   //TODO no buckets
      // We need recursion to calculate the rotation at the ending level.

      locTreeSort(points,
          tempSplitters[child_sfc], tempSplitters[child_sfc+1],
          sLev+1, eLev,
          cRot,
          outBuckets,
          makeBuckets);
    }
  }
  else if (makeBuckets)   //TODO Don't need this branch if no buckets.
  {
    // This is the ending level. Use the splitters to build the list of ending buckets.
    for (char child_sfc = 0; child_sfc < numChildren; child_sfc++)
    {
      ChildI child = rot_perm[child_sfc] - '0';     // Decode from human-readable ASCII.
      RotI cRot = orientLookup[child];

      if (tempSplitters[child_sfc+1] - tempSplitters[child_sfc] == 0)
        continue;

      //TODO remove
      outBuckets.push_back(
          {cRot, sLev+1,
          tempSplitters[child_sfc],
          tempSplitters[child_sfc+1]});
      // These are the parameters that could be used to further refine the bucket.
    }
  }

}// end function()


//
// SFC_bucketing()
//
//   Based on Dendro4 sfcSort.h SFC_bucketing().
//
template<typename T, unsigned int D>
void
SFC_Tree<T,D>:: SFC_bucketing(TreeNode<T,D> *points,
                          RankI begin, RankI end,
                          LevI lev,
                          RotI pRot,
                          std::array<RankI, 1+TreeNode<T,D>::numChildren> &outSplitters)
{
  // ==
  // Reorder the points by child number at level `lev', in the order
  // of the SFC, and yield the positions of the splitters.
  //
  // Higher-level nodes will be counted in the bucket for 0th children (SFC order).
  // ==

  using TreeNode = TreeNode<T,D>;
  constexpr char numChildren = TreeNode::numChildren;
  constexpr char rotOffset = 2*numChildren;  // num columns in rotations[].

  //
  // Count the number of points in each bucket,
  // indexed by (Morton) child number.
  std::array<int, numChildren> counts;
  counts.fill(0);
  int countAncestors = 0;   // Special bucket to ensure ancestors precede descendants.
  /// for (const TreeNode &tn : inp)
  for (const TreeNode *tn = points + begin; tn < points + end; tn++)
  {
    if (tn->getLevel() < lev)
      countAncestors++;
    else
      counts[tn->getMortonIndex(lev)]++;
  }

  //
  // Compute offsets of buckets in permuted SFC order.
  // Conceptually:
  //   1. Permute counts;  2. offsets=scan(counts);  3. Un-permute offsets.
  //
  // The `outSplitters' array is indexed in SFC order (to match final output),
  // while the `offsets' and `bucketEnds` arrays are indexed in Morton order
  // (for easy lookup using TreeNode.getMortonIndex()).
  //
  std::array<RankI, numChildren+1> offsets, bucketEnds;  // Last idx represents ancestors.
  offsets[numChildren] = begin;
  bucketEnds[numChildren] = begin + countAncestors;
  RankI accum = begin + countAncestors;                  // Ancestors belong in front.

  std::array<TreeNode, numChildren+1> unsortedBuffer;
  int bufferSize = 0;

  // Logically permute: Scan the bucket-counts in the order of the SFC.
  // Since we want to map [SFC_rank]-->Morton_rank,
  // use the "left" columns of rotations[], aka `rot_perm'.
  const ChildI *rot_perm = &rotations[pRot*rotOffset + 0*numChildren];
  ChildI child_sfc = 0;
  for ( ; child_sfc < numChildren; child_sfc++)
  {
    ChildI child = rot_perm[child_sfc] - '0';  // Decode from human-readable ASCII.
    outSplitters[child_sfc] = accum;
    offsets[child] = accum;           // Start of bucket. Moving marker.
    accum += counts[child];
    bucketEnds[child] = accum;        // End of bucket. Fixed marker.
  }
  outSplitters[child_sfc] = accum;  // Should be the end.
  outSplitters[0] = begin;          // Bucket for 0th child (SFC order) contains ancestors too.

  // Prepare for the in-place movement phase by copying each offsets[] pointee
  // to the rotation buffer. This frees up the slots to be valid destinations.
  // Includes ancestors: Recall, we have used index `numChildren' for ancestors.
  for (char bucketId = 0; bucketId <= numChildren; bucketId++)
  {
    if (offsets[bucketId] < bucketEnds[bucketId])
      unsortedBuffer[bufferSize++] = points[offsets[bucketId]];  // Copy TreeNode.
  }

  //
  // Finish the movement phase.
  //
  // Invariant: Any offsets[] pointee has been copied into `unsortedBuffer'.
  while (bufferSize > 0)
  {
    TreeNode *bufferTop = &unsortedBuffer[bufferSize-1];
    unsigned char destBucket
      = (bufferTop->getLevel() < lev) ? numChildren : bufferTop->getMortonIndex(lev);

    points[offsets[destBucket]++] = *bufferTop;  // Set down the TreeNode.

    // Follow the cycle by picking up the next element in destBucket...
    // unless we completed a cycle: in that case we made progress with unsortedBuffer.
    if (offsets[destBucket] < bucketEnds[destBucket])
      *bufferTop = points[offsets[destBucket]];    // Copy TreeNode.
    else
      bufferSize--;
  }
}



template<typename T, unsigned int D>
void
SFC_Tree<T,D>:: distTreeSort(std::vector<TreeNode<T,D>> &points,
                          double loadFlexibility,
                          MPI_Comm comm)
{

  // -- Don't worry about K splitters for now, we'll add that later. --

  // The goal of this function, as explained in Fernando and Sundar's paper,
  // is to refine the list of points into finer sorted buckets until
  // the balancing criterion has been met. Therefore the hyperoctree is
  // traversed in breadth-first order.
  //
  // I've considered two ways to do a breadth first traversal:
  // 1. Repeated depth-first traversal with a stack to hold rotations
  //    (requires storage linear in height of the tree, but more computation)
  // 2. Single breadth-first traversal with a queue to hold rotations
  //    (requires storage linear in the breadth of the tree, but done in a
  //    single pass of the tree. Also can take advantage of sparsity and filtering).
  // The second approach is used in Dendro4 par::sfcTreeSort(), so
  // I'm going to assume that linear aux storage is not too much to ask.

  int nProc, rProc;
  MPI_Comm_rank(comm, &rProc);
  MPI_Comm_size(comm, &nProc);

  if (nProc == 1)
  {
    auto unusedBucketVector = getEmptyBucketVector();
    locTreeSort(&(*points.begin()), 0, points.size(), 0, m_uiMaxDepth, 0, unusedBucketVector, false);
    return;
  }


  using TreeNode = TreeNode<T,D>;
  constexpr char numChildren = TreeNode::numChildren;
  constexpr char rotOffset = 2*numChildren;  // num columns in rotations[].

  // The outcome of the BFT will be a list of splitters, i.e. refined buckets.
  // As long as there are `pending splitters', we will refine their corresponding buckets.
  // Initially all splitters are pending.
  std::vector<RankI> splitters(nProc, 0);
  BarrierQueue<RankI> pendingSplitterIdx(nProc);
  for (RankI sIdx = 0; sIdx < nProc; sIdx++)
    pendingSplitterIdx.q[sIdx] = sIdx;
  pendingSplitterIdx.reset_barrier();

  //
  // Phase 1: move down the levels until we have enough buckets
  //   to test our load-balancing criterion.
  BarrierQueue<BucketInfo<RankI>> bftQueue;
  const int initNumBuckets = nProc;
  const BucketInfo<RankI> rootBucket = {0, 0, 0, (RankI) points.size()};
  bftQueue.q.push_back(rootBucket);
        // No-runaway, in case we run out of points.
        // It is `<' because refining m_uiMaxDepth would make (m_uiMaxDepth+1).
  while (bftQueue.q.size() < initNumBuckets && bftQueue.q[0].lev < m_uiMaxDepth)
  {
    treeBFTNextLevel(&(*points.begin()), bftQueue.q);
  }
  // Remark: Due to the no-runaway clause, we are not guaranteed
  // that bftQueue actually holds `initNumBuckets' buckets.

  //
  // Phase 2: Count bucket sizes, communicate bucket sizes,
  //   test load balance, select buckets and refine, repeat.

  RankI sizeG, sizeL = points.size();
  sizeG = sizeL;   // Proxy for summing, to test with one proc.
  par::Mpi_Allreduce<RankI>(&sizeL, &sizeG, 1, MPI_SUM, comm);


  /// //TEST  print all ideal splitters pictorally
  /// RankI oldLoc = 0;
  /// for (int rr = 0; rr < nProc; rr++)
  /// {
  ///   RankI idealSplitter = (rr+1) * sizeG / nProc;
  ///   for ( ; oldLoc < idealSplitter; oldLoc++) { std::cout << ' '; }
  ///   std::cout << 'I';     oldLoc++;
  /// }
  /// std::cout << '\n';

  std::vector<unsigned int> DBG_splitterIteration(nProc, 0);  //DEBUG

  // To avoid communicating buckets that did not get updated, we'll cycle:
  //   Select buckets from old queue, -> enqueue in a "new" queue, -> ...
  //   Use the class BarrierQueue to separate old from new.
  // I also group buckets into "blocks" of contiguous siblings before scanning.
  std::vector<RankI> bktCountsL, bktCountsG;  // As single-use containers per level.
  BarrierQueue<RankI> blkBeginG;              // As queue with barrier, to bridge levels.
  blkBeginG.enqueue(0);
  RankI blkNumBkt = bftQueue.size();  // The first block is special. It contains all buckets.
  while (pendingSplitterIdx.size() > 0)
  {
    /// // TEST Print all new buckets pictorally.
    /// { RankI DBG_oldLoc = 0;
    ///   for (BucketInfo<RankI> b : bftQueue.q) { while(DBG_oldLoc < b.end) { std::cout << ' '; DBG_oldLoc++; } if (DBG_oldLoc == b.end) { std::cout << '|'; DBG_oldLoc++; } }
    ///   std::cout << '\n';
    /// }

    bftQueue.reset_barrier();
    blkBeginG.reset_barrier();
    pendingSplitterIdx.reset_barrier();

    // Count buckets locally and globally.
    bktCountsL.clear();
    for (BucketInfo<RankI> b : bftQueue.leading())
      bktCountsL.push_back(b.end - b.begin);
    bktCountsG.resize(bktCountsL.size());
    bktCountsG = bktCountsL;              // Proxy for summing, to test with one proc.
    par::Mpi_Allreduce<RankI>(&(*bktCountsL.begin()), &(*bktCountsG.begin()), (int) bktCountsL.size(), MPI_SUM, comm);

    // Compute ranks, test load balance, and collect buckets for refinement.
    // Process each bucket in sequence, one block of buckets at a time.
    RankI bktBeginG;
    while (blkBeginG.dequeue(bktBeginG))
    {
      for (RankI ii = 0; ii < blkNumBkt; ii++)
      {
        const RankI bktCountG = bktCountsG[0];
        const RankI bktEndG = bktBeginG + bktCountG;
        bktCountsG.erase(bktCountsG.begin());

        BucketInfo<RankI> refBkt;
        bftQueue.dequeue(refBkt);
        bool selectBucket = false;
        const bool bktCanBeRefined = (refBkt.lev < m_uiMaxDepth);
        
        // Test the splitter indices that may fall into the current bucket.
        RankI idealSplitterG;
        while (pendingSplitterIdx.get_barrier()
            && (idealSplitterG = (pendingSplitterIdx.front()+1) * sizeG / nProc) <= bktEndG)
        {
          RankI r;
          pendingSplitterIdx.dequeue(r);
          RankI absTolerance = ((r+1) * sizeG / nProc - r * sizeG / nProc) * loadFlexibility;
          if (bktCanBeRefined && (bktEndG - idealSplitterG) > absTolerance)
          {
            // Too far. Mark bucket for refinment. Send splitter back to queue.
            selectBucket = true;
            pendingSplitterIdx.enqueue(r);
            splitters[r] = refBkt.end;      // Will be overwritten. Uncomment if want to see progress.
          }
          else
          {
            // Good enough. Accept the bucket by recording the local splitter.
            splitters[r] = refBkt.end;
          }
          DBG_splitterIteration[r]++;   //DEBUG
        }

        if (selectBucket)
        {
          bftQueue.enqueue(refBkt);      // For refinment.
          blkBeginG.enqueue(bktBeginG);  // Bucket-begin becomes block-begin on next iteration.
        }

        bktBeginG = bktEndG;
      }
    }

    /// // TEST Print all splitters.
    /// { RankI DBG_oldLoc = 0;
    ///   for (RankI s : splitters) { while(DBG_oldLoc < s) { std::cout << '_'; DBG_oldLoc++; }  if (DBG_oldLoc == s) { std::cout << 'x'; DBG_oldLoc++; } }
    ///   std::cout << '\n';
    /// }
    
    // Refine the buckets that we have set aside.
    treeBFTNextLevel(&(*points.begin()), bftQueue.q);

    blkNumBkt = numChildren;  // After the first level, blocks result from refining a single bucket.
  }

  /// // DEBUG: print out all the points.
  /// { std::vector<char> spaces(m_uiMaxDepth*rProc+1, ' ');
  /// spaces.back() = '\0';
  /// for (const TreeNode tn : points)
  ///   std::cout << spaces.data() << tn.getBase32Hex().data() << "\n";
  /// std::cout << spaces.data() << "------------------------------------\n";
  /// }

  //
  // All to all exchange of the points arrays.

  std::vector<unsigned int> sendCnt, sendDspl;
  std::vector<unsigned int> recvCnt(splitters.size()), recvDspl;
  sendCnt.reserve(splitters.size());
  sendDspl.reserve(splitters.size());
  recvDspl.reserve(splitters.size());
  RankI sPrev = 0;
  for (RankI s : splitters)     // Sequential counting and displacement.
  {
    sendDspl.push_back(sPrev);
    sendCnt.push_back(s - sPrev);
    sPrev = s;
  }
  par::Mpi_Alltoall<RankI>(&(*sendCnt.begin()), &(*recvCnt.begin()), 1, comm);
  sPrev = 0;
  for (RankI c : recvCnt)       // Sequential scan.
  {
    recvDspl.push_back(sPrev);
    sPrev += c;
  }
  RankI sizeNew = sPrev;

  std::vector<TreeNode> origPoints = points;   // Sendbuffer is a copy.

  if (sizeNew > sizeL)
    points.resize(sizeNew);

  par::Mpi_Alltoallv<TreeNode>(
      &(*origPoints.begin()), (int*) &(*sendCnt.begin()), (int*) &(*sendDspl.begin()),
      &(*points.begin()), (int*) &(*recvCnt.begin()), (int*) &(*recvDspl.begin()),
      comm);

  points.resize(sizeNew);

  /// // DEBUG: print out all the points.
  /// { std::vector<char> spaces(m_uiMaxDepth*rProc+1, ' ');
  /// spaces.back() = '\0';
  /// for (const TreeNode tn : points)
  ///   std::cout << spaces.data() << tn.getBase32Hex().data() << "\n";
  /// std::cout << spaces.data() << "------------------------------------\n";
  /// }

  //TODO figure out the 'staged' part with k-parameter.

  // Finish with a local TreeSort to ensure all points are in order.
  auto unusedBucketVector = getEmptyBucketVector();
  locTreeSort(&(*points.begin()), 0, points.size(), 0, m_uiMaxDepth, 0, unusedBucketVector, false);

  /// // DEBUG: print out all the points.
  /// { std::vector<char> spaces(m_uiMaxDepth*rProc+1, ' ');
  /// spaces.back() = '\0';
  /// for (const TreeNode tn : points)
  ///   std::cout << spaces.data() << tn.getBase32Hex().data() << "\n";
  /// std::cout << spaces.data() << "------------------------------------\n";
  /// }
}


template <typename T, unsigned int D>
void
SFC_Tree<T,D>:: treeBFTNextLevel(TreeNode<T,D> *points,
      std::vector<BucketInfo<RankI>> &bftQueue)
{
  if (bftQueue.size() == 0)
    return;

  const LevI startLev = bftQueue[0].lev;

  using TreeNode = TreeNode<T,D>;
  constexpr char numChildren = TreeNode::numChildren;
  constexpr char rotOffset = 2*numChildren;  // num columns in rotations[].

  while (bftQueue[0].lev == startLev)
  {
    BucketInfo<RankI> front = bftQueue[0];
    bftQueue.erase(bftQueue.begin());

    // Refine the current orthant/bucket by sorting the sub-buckets.
    // Get splitters for sub-buckets.
    std::array<RankI, numChildren+1> childSplitters;
    if (front.begin < front.end)
      SFC_bucketing(points, front.begin, front.end, front.lev, front.rot_id, childSplitters);
    else
      childSplitters.fill(front.begin);  // Don't need to sort an empty selection, it's just empty.

    // Enqueue our children in the next level.
    const ChildI * const rot_perm = &rotations[front.rot_id*rotOffset + 0*numChildren];
    const RotI * const orientLookup = &HILBERT_TABLE[front.rot_id*numChildren];
    for (char child_sfc = 0; child_sfc < numChildren; child_sfc++)
    {
      ChildI child = rot_perm[child_sfc] - '0';     // Decode from human-readable ASCII.
      RotI cRot = orientLookup[child];
      BucketInfo<RankI> childBucket =
          {cRot, front.lev+1, childSplitters[child_sfc], childSplitters[child_sfc+1]};

      bftQueue.push_back(childBucket);
    }
  }
}




} // namspace ot
