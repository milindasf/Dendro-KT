/**
 * @file:nsort.tcc
 * @author: Masado Ishii  --  UofU SoC,
 * @date: 2019-02-20
 */

namespace ot {

  // ==================== Begin: CellType ==================== //
 
  //
  // getExteriorOrientHigh2Low()
  //
  template <unsigned int OuterDim>
  std::array<CellType<OuterDim>, (1u<<OuterDim)-1>
  CellType<OuterDim>::getExteriorOrientHigh2Low()
  {
    std::array<CellType, (1u<<OuterDim)-1> orientations;
    CellType *dest = orientations.data();
    for (int fdim = OuterDim - 1; fdim >= 0; fdim--)
    {
      CellType *gpIter = dest;
      emitCombinations(0u, OuterDim, fdim, dest);
      while (gpIter < dest)
        (gpIter++)->set_dimFlag(fdim);
    }
    return orientations;
  }

  //
  // getExteriorOrientLow2High()
  //
  template <unsigned int OuterDim>
  std::array<CellType<OuterDim>, (1u<<OuterDim)-1>
  CellType<OuterDim>::getExteriorOrientLow2High()
  {
    std::array<CellType, (1u<<OuterDim)-1> orientations;
    CellType *dest = orientations.data();
    for (int fdim = 0; fdim < OuterDim; fdim++)
    {
      CellType *gpIter = dest;
      emitCombinations(0u, OuterDim, fdim, dest);
      while (gpIter < dest)
        (gpIter++)->set_dimFlag(fdim);
    }
    return orientations;
  }

  //
  // emitCombinations()
  //
  template <unsigned int OuterDim>
  void CellType<OuterDim>::emitCombinations(
      FlagType prefix, unsigned char lengthLeft, unsigned char onesLeft,
      CellType * &dest)
  {
    assert (onesLeft <= lengthLeft);

    if (onesLeft == 0)
      (dest++)->set_orientFlag(prefix | 0u);
    else if (onesLeft == lengthLeft)
      (dest++)->set_orientFlag(prefix | ((1u << lengthLeft) - 1u));
    else
    {
      emitCombinations(prefix, lengthLeft - 1, onesLeft, dest);
      emitCombinations(prefix | (1u << (lengthLeft - 1)), lengthLeft - 1, onesLeft - 1, dest);
    }
  }


  // ==================== End: CellType ==================== //


  // ============================ Begin: TNPoint === //

  /**
   * @brief Constructs a node at the extreme "lower-left" corner of the domain.
   */
  template <typename T, unsigned int dim>
  TNPoint<T,dim>::TNPoint() : TreeNode<T,dim>(), m_isSelected(Maybe)
  { }

  /**
    @brief Constructs a point.
    @param coords The coordinates of the point.
    @param level The level of the point (i.e. level of the element that spawned it).
    @note Uses the "dummy" overload of TreeNode() so that the coordinates are copied as-is.
    */
  template <typename T, unsigned int dim>
  TNPoint<T,dim>::TNPoint (const std::array<T,dim> coords, unsigned int level) :
      TreeNode<T,dim>(0, coords, level), m_isSelected(Maybe)
  { }

  /**@brief Copy constructor */
  template <typename T, unsigned int dim>
  TNPoint<T,dim>::TNPoint (const TNPoint<T,dim> & other) :
      TreeNode<T,dim>(other),
      m_isSelected(other.m_isSelected), m_numInstances(other.m_numInstances), m_owner(other.m_owner)
  { }

  /**
    @brief Constructs an octant (without checks).
    @param dummy : not used yet.
    @param coords The coordinates of the point.
    @param level The level of the point (i.e. level of the element that spawned it).
  */
  template <typename T, unsigned int dim>
  TNPoint<T,dim>::TNPoint (const int dummy, const std::array<T,dim> coords, unsigned int level) :
      TreeNode<T,dim>(dummy, coords, level), m_isSelected(Maybe)
  { }

  /** @brief Assignment operator. No checks for dim or maxD are performed. It's ok to change dim and maxD of the object using the assignment operator.*/
  template <typename T, unsigned int dim>
  TNPoint<T,dim> & TNPoint<T,dim>::operator = (TNPoint<T,dim> const  & other)
  {
    TreeNode<T,dim>::operator=(other);
    m_isSelected = other.m_isSelected;
    m_numInstances = other.m_numInstances;
    m_owner = other.m_owner;
  }


  template <typename T, unsigned int dim>
  bool TNPoint<T,dim>::operator== (TNPoint const &that) const
  {
    std::array<T,dim> coords1, coords2;
    return TreeNode<T,dim>::getLevel() == that.getLevel() && (TreeNode<T,dim>::getAnchor(coords1), coords1) == (that.getAnchor(coords2), coords2);
  }

  template <typename T, unsigned int dim>
  bool TNPoint<T,dim>::operator!= (TNPoint const &that) const
  {
    return ! operator==(that);
  }


  template <typename T, unsigned int dim>
  unsigned char TNPoint<T,dim>::get_firstIncidentHyperplane(unsigned int hlev) const
  {
    using TreeNode = TreeNode<T,dim>;
    const unsigned int len = 1u << (m_uiMaxDepth - hlev);
    const unsigned int interiorMask = len - 1;

    for (int d = 0; d < dim; d++)
    {
      bool axisIsInVolume = TreeNode::m_uiCoords[d] & interiorMask;
      if (!axisIsInVolume)   // Then the point is on the hyperplane face normal to this axis.
        return d;
    }
    return dim;
  }

  /**@brief Infer the type (dimension and orientation) of cell this point is interior to, from coordinates and level. */
  template <typename T, unsigned int dim>
  CellType<dim> TNPoint<T,dim>::get_cellType() const
  {
    return get_cellType(TreeNode<T,dim>::m_uiLevel);
  }

  template <typename T, unsigned int dim>
  CellType<dim> TNPoint<T,dim>::get_cellTypeOnParent() const
  {
    return get_cellType(TreeNode<T,dim>::m_uiLevel - 1);
  }

  template <typename T, unsigned int dim>
  CellType<dim> TNPoint<T,dim>::get_cellType(LevI lev) const
  {
    using TreeNode = TreeNode<T,dim>;
    const unsigned int len = 1u << (m_uiMaxDepth - lev);
    const unsigned int interiorMask = len - 1;

    unsigned char cellDim = 0u;
    unsigned char cellOrient = 0u;
    #pragma unroll(dim)
    for (int d = 0; d < dim; d++)
    {
      bool axisIsInVolume = TreeNode::m_uiCoords[d] & interiorMask;
      cellOrient |= (unsigned char) axisIsInVolume << d;
      cellDim += axisIsInVolume;
    }

    CellType<dim> cellType;
    cellType.set_dimFlag(cellDim);
    cellType.set_orientFlag(cellOrient);

    return cellType;
  }


  template <typename T, unsigned int dim>
  bool TNPoint<T,dim>::isCrossing() const
  {
    using TreeNode = TreeNode<T,dim>;
    const unsigned int len = 1u << (m_uiMaxDepth - TreeNode::m_uiLevel);
    const unsigned int mask = (len << 1u) - 1u;
    for (int d = 0; d < dim; d++)
      if ((TreeNode::m_uiCoords[d] & mask) == len)
        return true;
    return false;
  }

  template <typename T, unsigned int dim>
  TreeNode<T,dim> TNPoint<T,dim>::getFinestOpenContainer() const
  {
    assert((!TreeNode<T,dim>::isOnDomainBoundary()));  // When we bucket we need to set aside boundary points first.

    //
    // A node is on a boundary at level lev iff any coordinates have only zeros
    // for all bits strictly deeper than lev. Take the contrapositive:
    // A node is NOT on a boundary at level lev iff all coordinates have at least
    // one nonzero bit strictly deeper than lev. We need the deepest such level.
    // To get the deepest such level, find the deepest nonzero bit in each
    // coordinate, and take the coarsest of finest levels.
    //
    using TreeNode = TreeNode<T,dim>;
    unsigned int coarsestFinestHeight = 0;
    for (int d = 0; d < dim; d++)
    {
      unsigned int finestHeightDim = binOp::lowestOnePos(TreeNode::m_uiCoords[d]);
      // Maximum height is minimum depth.
      if (finestHeightDim > coarsestFinestHeight)
        coarsestFinestHeight = finestHeightDim;
    }
    unsigned int level = m_uiMaxDepth - coarsestFinestHeight; // Convert to depth.
    level--;                           // Nonzero bit needs to be strictly deeper.

    // Use the non-dummy overload so that the coordinates get clipped.
    return TreeNode(TreeNode::m_uiCoords, level);
  }

  template <typename T, unsigned int dim>
  TreeNode<T,dim> TNPoint<T,dim>::getCell() const
  {
    using TreeNode = TreeNode<T,dim>;
    return TreeNode(TreeNode::m_uiCoords, TreeNode::m_uiLevel);  // Truncates coordinates to cell anchor.
  }

  template <typename T, unsigned int dim>
  void TNPoint<T,dim>::appendAllBaseNodes(std::vector<TNPoint> &nodeList)
  {
    // The base nodes are obtained by adding or subtracting `len' in every
    // normal axis (self-boundary axis) and scaling x2 the off-anchor offset in every
    // tangent axis (self-interior axis). Changes are only made for the
    // parent-interior axes.
    // In other words, scaling x2 the offset from every vertex of the cellTypeOnParent.
    // Note: After scaling, may have rounding artifacts!
    // Must compare within a +/- distance of the least significant bit.
    // Will at least end up on the same interface/hyperplane, and should
    // be very close by in SFC order.

    using TreeNode = TreeNode<T,dim>;
    const unsigned int parentLen = 1u << (m_uiMaxDepth - (TreeNode::m_uiLevel - 1));
    const unsigned int interiorMask = parentLen - 1;

    std::array<T,dim> anchor;
    getCell().getParent().getAnchor(anchor);

    std::array<unsigned char, dim> interiorAxes;
    unsigned char celldim = 0;
    for (int d = 0; d < dim; d++)
    {
      if (TreeNode::m_uiCoords[d] & interiorMask)
        interiorAxes[celldim++] = d;
    }

    for (unsigned char vId = 0; vId < (1u << celldim); vId++)
    {
      TNPoint<T,dim> base(*this);
      base.setLevel(TreeNode::m_uiLevel - 1);
      for (int dIdx = 0; dIdx < celldim; dIdx++)
      {
        int d = interiorAxes[dIdx];
        T vtxCoord = anchor[d] + (vId & (1u << dIdx) ? parentLen : 0);
        base.setX(d, ((base.getX(d) - vtxCoord) << 1) + vtxCoord);
      }

      nodeList.push_back(base);
    }
  }

  // ============================ End: TNPoint ============================ //


  // ============================ Begin: Element ============================ //

  template <typename T, unsigned int dim>
  void Element<T,dim>::appendNodes(unsigned int order, std::vector<TNPoint<T,dim>> &nodeList) const
  {
    using TreeNode = TreeNode<T,dim>;
    const unsigned int len = 1u << (m_uiMaxDepth - TreeNode::m_uiLevel);

    const unsigned int numNodes = intPow(order+1, dim);

    std::array<unsigned int, dim> nodeIndices;
    nodeIndices.fill(0);
    for (unsigned int node = 0; node < numNodes; node++)
    {
      std::array<T,dim> nodeCoords;
      #pragma unroll(dim)
      for (int d = 0; d < dim; d++)
        nodeCoords[d] = len * nodeIndices[d] / order  +  TreeNode::m_uiCoords[d];
      nodeList.push_back(TNPoint<T,dim>(nodeCoords, TreeNode::m_uiLevel));

      incrementBaseB<unsigned int, dim>(nodeIndices, order+1);
    }
  }


  template <typename T, unsigned int dim>
  void Element<T,dim>::appendInteriorNodes(unsigned int order, std::vector<TNPoint<T,dim>> &nodeList) const
  {
    // Basically the same thing as appendNodes (same dimension of volume, if nonempty),
    // just use (order-1) instead of (order+1), and shift indices by 1.
    using TreeNode = TreeNode<T,dim>;
    const unsigned int len = 1u << (m_uiMaxDepth - TreeNode::m_uiLevel);

    const unsigned int numNodes = intPow(order-1, dim);

    std::array<unsigned int, dim> nodeIndices;
    nodeIndices.fill(0);
    for (unsigned int node = 0; node < numNodes; node++)
    {
      std::array<T,dim> nodeCoords;
      #pragma unroll(dim)
      for (int d = 0; d < dim; d++)
        nodeCoords[d] = len * (nodeIndices[d]+1) / order  +  TreeNode::m_uiCoords[d];
      nodeList.push_back(TNPoint<T,dim>(nodeCoords, TreeNode::m_uiLevel));

      incrementBaseB<unsigned int, dim>(nodeIndices, order-1);
    }
  }

  template <typename T, unsigned int dim>
  void Element<T,dim>::appendExteriorNodes(unsigned int order, std::vector<TNPoint<T,dim>> &nodeList) const
  {
    // Duplicated the appendNodes() function, and then caused every interior node to be thrown away.
    using TreeNode = TreeNode<T,dim>;
    const unsigned int len = 1u << (m_uiMaxDepth - TreeNode::m_uiLevel);

    const unsigned int numNodes = intPow(order+1, dim);

    std::array<unsigned int, dim> nodeIndices;
    nodeIndices.fill(0);
    for (unsigned int node = 0; node < numNodes; node++)
    {
      // For exterior, need at least one of the entries in nodeIndices to be 0 or order.
      if (std::count(nodeIndices.begin(), nodeIndices.end(), 0) == 0 &&
          std::count(nodeIndices.begin(), nodeIndices.end(), order) == 0)
      {
        nodeIndices[0] = order;   // Skip ahead to the lexicographically next boundary node.
        node += order - 1;
      }

      std::array<T,dim> nodeCoords;
      #pragma unroll(dim)
      for (int d = 0; d < dim; d++)
        nodeCoords[d] = len * nodeIndices[d] / order  +  TreeNode::m_uiCoords[d];
      nodeList.push_back(TNPoint<T,dim>(nodeCoords, TreeNode::m_uiLevel));

      incrementBaseB<unsigned int, dim>(nodeIndices, order+1);
    }
  }


  template <typename T, unsigned int dim>
  void Element<T,dim>::appendKFaces(CellType<dim> kface,
      std::vector<TreeNode<T,dim>> &nodeList, std::vector<CellType<dim>> &kkfaces) const
  {
    using TreeNode = TreeNode<T,dim>;
    const unsigned int len = 1u << (m_uiMaxDepth - TreeNode::m_uiLevel);

    unsigned int fdim = kface.get_dim_flag();
    unsigned int orient = kface.get_orient_flag();

    const unsigned int numNodes = intPow(3, fdim);

    std::array<unsigned int, dim> nodeIndices;
    nodeIndices.fill(0);
    for (unsigned int node = 0; node < numNodes; node++)
    {
      unsigned char kkfaceDim = 0;
      unsigned int  kkfaceOrient = 0u;
      std::array<T,dim> nodeCoords = TreeNode::m_uiCoords;
      int vd = 0;
      for (int d = 0; d < dim; d++)
      {
        if (orient & (1u << d))
        {
          if (nodeIndices[vd] == 1)
          {
            kkfaceDim++;
            kkfaceOrient |= (1u << d);
          }
          else if (nodeIndices[vd] == 2)
            nodeCoords[d] += len;

          vd++;
        }
      }
      nodeList.push_back(TreeNode(nodeCoords, TreeNode::m_uiLevel));
      kkfaces.push_back(CellType<dim>());
      kkfaces.back().set_dimFlag(kkfaceDim);
      kkfaces.back().set_orientFlag(kkfaceOrient);

      incrementBaseB<unsigned int, dim>(nodeIndices, 3);
    }
  }


  // ============================ End: Element ============================ //


  // ============================ Begin: ScatterFace ============================ //

  template <typename T, unsigned int dim>
  void ScatterFace<T,dim>::sortUniq(std::vector<ScatterFace> &faceList)
  {
    // Sort so that unique-location points end up adjacent in faceList.
    // Depends on having the special case for domain level integrated into locTreeSort.
    SFC_Tree<T,dim>::template locTreeSort<ScatterFace>(&(*faceList.begin()), 0, (RankI) faceList.size(), 0, m_uiMaxDepth, 0);

    std::unordered_set<int> uniqueOwners;

    // Shuffle and copy by owner.
    typename std::vector<ScatterFace>::iterator iterLead = faceList.begin(), iterFollow = faceList.begin();
    while (iterLead < faceList.end())
    {
      // Find group of unique point locations.
      typename std::vector<ScatterFace>::iterator gpBegin = iterLead, gpEnd = iterLead;
      while (gpEnd < faceList.end() && *gpEnd == *gpBegin)
        gpEnd++;

      // Collect set of unique owners within the group.
      uniqueOwners.clear();
      while (iterLead < gpEnd)
      {
        uniqueOwners.insert(iterLead->get_owner());
        iterLead++;
      }

      ScatterFace prototype = *gpBegin;

      // Write the set of unique owners into the leading points.
      // Only advance iterFollow as far as the number of unique owners.
      for (int o : uniqueOwners)
      {
        *iterFollow = prototype;
        iterFollow->set_owner(o);
        iterFollow++;
      }
    }

    // Erase the unused elements.
    faceList.erase(iterFollow, faceList.end());
  }

  // ============================ End: ScatterFace ============================ //


  // ============================ Begin: SFC_NodeSort ============================ //

  //
  // SFC_NodeSort::dist_countCGNodes()
  //
  template <typename T, unsigned int dim>
  RankI SFC_NodeSort<T,dim>::dist_countCGNodes(std::vector<TNPoint<T,dim>> &points, unsigned int order, const TreeNode<T,dim> *treePartStart, MPI_Comm comm)
  {
    using TNP = TNPoint<T,dim>;

    // Counting:
    // 1. Call countCGNodes(classify=false) to agglomerate node instances -> node representatives.
    // 2. For every representative, determine which processor boundaries the node is incident on,
    //    by generating neighbor-keys and sorting them against the proc-bdry splitters.
    // 3. Each representative is copied once per incident proc-boundary.
    // 4. Send and receive the boundary layer nodes.
    // 5. Finally, sort and count nodes with countCGNodes(classify=true).

    // Scattermap:
    // 1. Before finalizing ownership of nodes and compacting the node list,
    //    convert both hanging and non-hanging neighbour nodes into a set of
    //    "scatterfaces."
    //    - The scatterfaces are open k-faces that may contain owned nodes.
    // 2. Build the scattermap from the owned nodes that are contained by scatterfaces.

    int nProc, rProc;
    MPI_Comm_rank(comm, &rProc);
    MPI_Comm_size(comm, &nProc);

    if (nProc == 1)
      return countCGNodes(&(*points.begin()), &(*points.end()), order, true);

    if (points.size() == 0)
      return 0;

    // First local pass: Don't classify, just sort and count instances.
    countCGNodes(&(*points.begin()), &(*points.end()), order, false);

    // Compact node list (remove literal duplicates).
    RankI numUniquePoints = 0;
    for (const TNP &pt : points)
    {
      if (pt.get_numInstances() > 0)
        points[numUniquePoints++] = pt;
    }
    points.resize(numUniquePoints);


    //
    // Preliminary sharing information. Prepare to test for globally hanging nodes.
    // For each node (unique) node, find out if it's a proc-boundary node, and who shares.
    //
    struct BdryNodeInfo { RankI ptIdx; int numProcNb; };
    std::vector<BdryNodeInfo> bdryNodeInfo;
    std::vector<int> shareLists;

    std::vector<RankI> sendCounts(nProc, 0);
    std::vector<RankI> recvCounts(nProc, 0);

    std::vector<RankI> sendOffsets;
    std::vector<RankI> recvOffsets;
    std::vector<int> sendProc;
    std::vector<int> recvProc;
    std::vector<RankI> shareMap;

    // Get neighbour information.
    std::vector<TreeNode<T,dim>> splitters = dist_bcastSplitters(treePartStart, comm);
    assert((splitters.size() == nProc));
    for (RankI ptIdx = 0; ptIdx < numUniquePoints; ptIdx++)
    {
      // Concatenates list of (unique) proc neighbours to shareLists.
      int numProcNb = getProcNeighbours(points[ptIdx], splitters.data(), nProc, shareLists, 1);
      //TODO test the assertion that all listed proc-neighbours in a sublist are unique.

      // Remove ourselves from neighbour list.
      if (std::remove(shareLists.end() - numProcNb, shareLists.end(), rProc) < shareLists.end())
      {
        shareLists.erase(shareLists.end() - 1);
        numProcNb--;
      }

      if (numProcNb > 0)  // Shared, i.e. is a proc-boundary node.
      {
        bdryNodeInfo.push_back({ptIdx, numProcNb});    // Record which point and how many proc-neighbours.
        for (int nb = numProcNb; nb >= 1; nb--)
        {
          sendCounts[*(shareLists.end() - nb)]++;
        }
      }
    }


    // Create the preliminary send buffer, ``share buffer.''
    // We compute the preliminary send buffer only once, so don't make a separate scatter map for it.
    std::vector<TNP> shareBuffer;
    int sendTotal = 0;
    sendOffsets.resize(nProc);
    for (int proc = 0; proc < nProc; proc++)
    {
      sendOffsets[proc] = sendTotal;
      sendTotal += sendCounts[proc];
    }
    shareBuffer.resize(sendTotal);

    // Determine the receive counts, via Alltoall of send counts.
    par::Mpi_Alltoall(sendCounts.data(), recvCounts.data(), 1, comm); 

    // Copy outbound data into the share buffer.
    // Note: Advances sendOffsets, so will need to recompute sendOffsets later.
    const int *shareListPtr = &(*shareLists.begin());
    for (const BdryNodeInfo &nodeInfo : bdryNodeInfo)
    {
      for (int ii = 0; ii < nodeInfo.numProcNb; ii++)
      {
        int proc = *(shareListPtr++);
        points[nodeInfo.ptIdx].set_owner(rProc);   // Reflected in both copies, else default -1.
        shareBuffer[sendOffsets[proc]++] = points[nodeInfo.ptIdx];
      }
    }

    // Compact lists of neighbouring processors and (re)compute share offsets.
    // Note: This should only be done after we are finished with shareLists.
    int numSendProc = 0;
    int numRecvProc = 0;
    sendTotal = 0;
    int recvTotal = 0;
    sendOffsets.clear();
    for (int proc = 0; proc < nProc; proc++)
    {
      if (sendCounts[proc] > 0)   // Discard empty counts, which includes ourselves.
      {
        sendCounts[numSendProc++] = sendCounts[proc];  // Compacting.
        sendOffsets.push_back(sendTotal);
        sendProc.push_back(proc);
        sendTotal += sendCounts[proc];
      }
      if (recvCounts[proc] > 0)   // Discard empty counts, which includes ourselves.
      {
        recvCounts[numRecvProc++] = recvCounts[proc];  // Compacting.
        recvOffsets.push_back(recvTotal);
        recvProc.push_back(proc);
        recvTotal += recvCounts[proc];
      }

    }
    sendCounts.resize(numSendProc);
    recvCounts.resize(numRecvProc);

    // Preliminary receive will be into end of existing node list.
    points.resize(numUniquePoints + recvTotal);


    //
    // Send and receive. Sends and receives may not be symmetric.
    //
    std::vector<MPI_Request> requestSend(numSendProc);
    std::vector<MPI_Request> requestRecv(numRecvProc);
    MPI_Status status;

    // Send data.
    for (int sIdx = 0; sIdx < numSendProc; sIdx++)
      par::Mpi_Isend(shareBuffer.data() + sendOffsets[sIdx], sendCounts[sIdx], sendProc[sIdx], 0, comm, &requestSend[sIdx]);

    // Receive data.
    for (int rIdx = 0; rIdx < numRecvProc; rIdx++)
      par::Mpi_Irecv(points.data() + numUniquePoints + recvOffsets[rIdx], recvCounts[rIdx], recvProc[rIdx], 0, comm, &requestRecv[rIdx]);

    // Wait for sends and receives.
    for (int sIdx = 0; sIdx < numSendProc; sIdx++)
      MPI_Wait(&requestSend[sIdx], &status);
    for (int rIdx = 0; rIdx < numRecvProc; rIdx++)
      MPI_Wait(&requestRecv[rIdx], &status);



    // Second local pass, classifying nodes as hanging or non-hanging.
    countCGNodes(&(*points.begin()), &(*points.end()), order, true);


    //TODO For a more sophisticated version, could sort just the new points (fewer),
    //     then merge (faster) two sorted lists before applying the resolver.


    // 2019-03-07
    //   Today @masado and @milinda decided that a cell-decomposition
    //   approach would be cleaner than trying to somehow do a single-pass streaming
    //   node-node comparison. For one thing, hanging nodes can be quite distant
    //   from their pointed-to nodes. For another thing, cell-decomposition
    //   is exact, and so avoids rounding problems.
    //
    //   The decision to go with cell-decomposition leads to
    //   the following "scatterfaces."

    //
    // "Scatter-faces"
    //
    // Convert sets of neighbouring nodes into sets of neighbouring (closed) k-faces.
    // (One nonempty set per neighbouring processor).
    // > Non-hanging nodes --> own k-face.
    // > Hanging nodes ------> parent k-face.
    //
    // Afterward, decompose the closed k-faces into constituent open k'-faces.
    //

    // Generate lists of closed k-faces.
    ScatterFacesCollection kfaces;
    {
      typename std::vector<TNP>::iterator ptIter = points.begin();
      while (ptIter < points.end())
      {
        // Find bounds on group.
        const typename std::vector<TNP>::iterator gpStart = ptIter;
        typename std::vector<TNP>::iterator gpEnd = ptIter;
        while (gpEnd < points.end() && *gpEnd == *gpStart)
          gpEnd++;

        if (ptIter->get_isSelected() == TNP::Yes)  // Non-hanging.
        {
          while (ptIter < gpEnd)
          {
            if (ptIter->get_owner() != rProc && ptIter->get_owner() != -1)
            {
              const unsigned char orientation = ptIter->get_cellType().get_orient_flag();
              const TreeNode<T,dim> cell = ptIter->getCell();
              kfaces[orientation].push_back({cell, ptIter->get_owner()});
            }
            ptIter++;
          }
        }
        else if (!ptIter->isCrossing())            // Hanging and non-crossing.
        {
          while (ptIter < gpEnd)
          {
            if (ptIter->get_owner() != rProc && ptIter->get_owner() != -1)
            {
              const unsigned char orientation = ptIter->get_cellTypeOnParent().get_orient_flag();
              const TreeNode<T,dim> cell = ptIter->getCell().getParent();
              kfaces[orientation].push_back({cell, ptIter->get_owner()});
            }
            ptIter++;
          }
        }

        ptIter = gpEnd;
      }
    }

    // De-clutter the lists of closed k-faces.
    for (std::vector<ScatterFace<T,dim>> &faceList : kfaces)
      ScatterFace<T,dim>::sortUniq(faceList);

    // Decompose the closed k-faces into constituent open k'-faces.
    /// auto orientationsHigh2Low = CellType<dim>::getExteriorOrientHigh2Low();
    auto cellsLow2High = CellType<dim>::getExteriorOrientLow2High();
    for (const CellType<dim> &cellType : cellsLow2High)
    {
      const unsigned int orientFlag = cellType.get_orient_flag();
      for (const ScatterFace<T,dim> &closedFace : kfaces[orientFlag])
      {
        std::vector<TreeNode<T,dim>> openFaces;
        std::vector<CellType<dim>> openCellTypes;
        Element<T,dim>(closedFace).appendKFaces(cellType, openFaces, openCellTypes);
        for (int f = 0; f < openFaces.size(); f++)
        {
          if (openCellTypes[f].get_orient_flag() != orientFlag)
            kfaces[openCellTypes[f].get_orient_flag()].push_back({openFaces[f], closedFace.get_owner()});
          // Open part of current closed kface is already represented in current list.
        }
      }
    }

    // De-clutter the lists of closed k-faces again. Now it matters that they're sorted.
    for (std::vector<ScatterFace<T,dim>> &faceList : kfaces)
      ScatterFace<T,dim>::sortUniq(faceList);


    //
    // Compute the scattermap from the scatterfaces and the owned nodes.
    //
    // > For every non-hanging ("Yes") boundary (owner != -1) node, determine ownership.
    //   Filter/compact the node list to owned non-boundary and boundary nodes.
    //
    // > Perform a top-down dual-traversal of
    //   - the sorted list of owned nodes, and
    //   - the sorted lists of scatterfaces.
    //
    //   During traversal, we should find a set of lowerleft nodes, per TreeNode,
    //   and at most one scatterface per neighbour per kface orientation, per TreeNode.
    //   For each node, look up its open CellType, and check if there are open scatterfaces
    //   in which the node is contained. If so, it means we need to add the node
    //   to the scattermap, once per containing scatterface, with different neighbours.
    //   We can also filter out non-boundary nodes. [We fixed the generate keys, so this is right.]
    //

    // Mark owned nodes and finalize the counting part.
    // Current ownership policy: Least-rank-processor (without the Base tag).
    RankI numOwnedPoints = 0;
    {
      typename std::vector<TNP>::iterator ptIter = points.begin();
      while (ptIter < points.end())
      {
        // Skip hanging nodes.
        if (ptIter->get_isSelected() != TNP::Yes)
        {
          ptIter++;
          continue;
        }

        // A node marked 'Yes' is the first instance.
        // Examine all instances and choose the one with least rank.
        typename std::vector<TNP>::iterator leastRank = ptIter;
        while (ptIter < points.end() && *ptIter == *leastRank)  // Equality compares only coordinates and level.
        {
          ptIter->set_isSelected(TNP::No);
          if (ptIter->get_owner() < leastRank->get_owner())
            leastRank = ptIter;
          ptIter++;
        }

        // If the chosen node is ours, select it and increment count.
        if (leastRank->get_owner() == -1 || leastRank->get_owner() == rProc)
        {
          leastRank->set_isSelected(TNP::Yes);
          numOwnedPoints++;
        }
      }
    }

    RankI numCGNodes = 0;  // The return variable for counting.
    par::Mpi_Allreduce(&numOwnedPoints, &numCGNodes, 1, MPI_SUM, comm);

    // Dual traversal to collect subset of owned nodes for the scattermap.
    // Main reason for using a separate function is the recursive depth-first traversal.
    ScatterMap scatterMap = computeScattermap(points, kfaces);

    return numCGNodes;
  }


  //
  // SFC_NodeSort::countCGNodes()
  //
  template <typename T, unsigned int dim>
  RankI SFC_NodeSort<T,dim>::countCGNodes(TNPoint<T,dim> *start, TNPoint<T,dim> *end, unsigned int order, bool classify)
  {
    using TNP = TNPoint<T,dim>;
    constexpr char numChildren = TreeNode<T,dim>::numChildren;
    RankI totalUniquePoints = 0;
    RankI numDomBdryPoints = filterDomainBoundary(start, end);

    if (!(end > start))
      return 0;

    // Sort the domain boundary points. Root-1 level requires special handling.
    //
    std::array<RankI, numChildren+1> rootSplitters;
    RankI unused_ancStart, unused_ancEnd;
    SFC_Tree<T,dim>::template SFC_bucketing_impl<KeyFunIdentity_Pt<TNP>, TNP, TNP>(
        end-numDomBdryPoints, 0, numDomBdryPoints, 0, 0,
        KeyFunIdentity_Pt<TNP>(), false, true,
        rootSplitters,
        unused_ancStart, unused_ancEnd);
    for (char child_sfc = 0; child_sfc < numChildren; child_sfc++)
    {
      if (rootSplitters[child_sfc+1] - rootSplitters[child_sfc+0] <= 1)
        continue;

      locTreeSortAsPoints(
          end-numDomBdryPoints, rootSplitters[child_sfc+0], rootSplitters[child_sfc+1],
          1, m_uiMaxDepth, 0);    // Re-use the 0th rotation for each 'root'.
    }

    // Counting/resolving/marking task.
    if (classify)
    {
      // Count the domain boundary points.
      //
      for (TNP *bdryIter = end - numDomBdryPoints; bdryIter < end; bdryIter++)
        bdryIter->set_isSelected(TNP::No);
      RankI numUniqBdryPoints = 0;
      TNP *bdryIter = end - numDomBdryPoints;
      TNP *firstCoarsest, *unused_firstFinest;
      unsigned int unused_numDups;
      while (bdryIter < end)
      {
        scanForDuplicates(bdryIter, end, firstCoarsest, unused_firstFinest, bdryIter, unused_numDups);
        firstCoarsest->set_isSelected(TNP::Yes);
        numUniqBdryPoints++;
      }

      totalUniquePoints += numUniqBdryPoints;


      // Bottom-up counting interior points
      //
      if (order <= 2)
        totalUniquePoints += countCGNodes_impl(resolveInterface_lowOrder, start, end - numDomBdryPoints, 1, 0, order);
      else
        totalUniquePoints += countCGNodes_impl(resolveInterface_highOrder, start, end - numDomBdryPoints, 1, 0, order);
    }
    // Sorting/instancing task.
    else
    {
      countInstances(end - numDomBdryPoints, end, order);
      countCGNodes_impl(countInstances, start, end - numDomBdryPoints, 1, 0, order);
    }

    return totalUniquePoints;
  }


  //
  // SFC_NodeSort::filterDomainBoundary()
  //
  template <typename T, unsigned int dim>
  RankI SFC_NodeSort<T,dim>::filterDomainBoundary(TNPoint<T,dim> *start, TNPoint<T,dim> *end)
  {
    using TNP = TNPoint<T,dim>;

    // Counting phase.
    RankI numDomBdryPoints = 0;
    std::queue<TNP *> segSplitters;
    std::vector<TNP> bdryPts;
    for (TNP *iter = start; iter < end; iter++)
    {
      if (iter->isOnDomainBoundary())
      {
        numDomBdryPoints++;
        segSplitters.push(iter);
        bdryPts.push_back(*iter);
      }
    }

    // Movement phase.
    TNP *writepoint;
    if (!segSplitters.empty())
      writepoint = segSplitters.front();
    while (!segSplitters.empty())
    {
      const TNP * const readstart = segSplitters.front() + 1;
      segSplitters.pop();
      const TNP * const readstop = (!segSplitters.empty() ? segSplitters.front() : end);
      // Shift the next segment of non-boundary points.
      for (const TNP *readpoint = readstart; readpoint < readstop; readpoint++)
        *(writepoint++) = *readpoint;
    }

    for (const TNP &bdryPt : bdryPts)
    {
      *(writepoint++) = bdryPt;
    }

    return numDomBdryPoints;
  }


  //
  // SFC_NodeSort::locTreeSortAsPoints()
  //
  template<typename T, unsigned int dim>
  void
  SFC_NodeSort<T,dim>:: locTreeSortAsPoints(TNPoint<T,dim> *points,
                            RankI begin, RankI end,
                            LevI sLev,
                            LevI eLev,
                            RotI pRot)
  {
    using TNP = TNPoint<T,dim>;
    constexpr char numChildren = TreeNode<T,dim>::numChildren;
    constexpr unsigned int rotOffset = 2*numChildren;  // num columns in rotations[].

    // Lookup tables to apply rotations.
    const ChildI * const rot_perm = &rotations[pRot*rotOffset + 0*numChildren];
    const RotI * const orientLookup = &HILBERT_TABLE[pRot*numChildren];

    if (end <= begin) { return; }

    // Reorder the buckets on sLev (current level).
    std::array<RankI, numChildren+1> tempSplitters;
    RankI unused_ancStart, unused_ancEnd;
    SFC_Tree<T,dim>::template SFC_bucketing_impl<KeyFunIdentity_Pt<TNP>, TNP, TNP>(
        points, begin, end, sLev, pRot,
        KeyFunIdentity_Pt<TNP>(), false, true,
        tempSplitters,
        unused_ancStart, unused_ancEnd);

    if (sLev < eLev)  // This means eLev is further from the root level than sLev.
    {
      // Recurse.
      // Use the splitters to specify ranges for the next level of recursion.
      for (char child_sfc = 0; child_sfc < numChildren; child_sfc++)
      {
        // Check for empty or singleton bucket.
        if (tempSplitters[child_sfc+1] - tempSplitters[child_sfc+0] <= 1)
          continue;

        ChildI child = rot_perm[child_sfc];
        RotI cRot = orientLookup[child];

        // Check for identical coordinates.
        bool allIdentical = true;
        std::array<T,dim> first_coords;
        points[tempSplitters[child_sfc+0]].getAnchor(first_coords);
        for (RankI srchIdentical = tempSplitters[child_sfc+0] + 1;
            srchIdentical < tempSplitters[child_sfc+1];
            srchIdentical++)
        {
          std::array<T,dim> other_coords;
          points[srchIdentical].getAnchor(other_coords);
          if (other_coords != first_coords)
          {
            allIdentical = false;
            break;
          }
        }

        if (!allIdentical)
        {
          locTreeSortAsPoints(points,
              tempSplitters[child_sfc+0], tempSplitters[child_sfc+1],
              sLev+1, eLev,
              cRot);
        }
        else
        {
          // Separate points by level. Assume at most 2 levels present,
          // due to 2:1 balancing. The first point determines which level is preceeding.
          const RankI segStart = tempSplitters[child_sfc+0];
          const RankI segSize = tempSplitters[child_sfc+1] - segStart;
          LevI firstLevel = points[segStart].getLevel();
          RankI ptIdx = 0;
          RankI ptOffsets[2] = {0, 0};
          while (ptIdx < segSize)
            if (points[segStart + (ptIdx++)].getLevel() == firstLevel)
              ptOffsets[1]++;

          if (ptOffsets[1] != 1 && ptOffsets[1] != segSize)
          {
            RankI ptEnds[2] = {ptOffsets[1], segSize};

            TNP buffer[2];
            ptOffsets[0]++;  // First point already in final position.
            buffer[0] = points[segStart + ptOffsets[0]];
            buffer[1] = points[segStart + ptOffsets[1]];
            unsigned char bufferSize = 2;

            while (bufferSize > 0)
            {
              TNP &bufferTop = buffer[bufferSize-1];
              unsigned char destBucket = !(bufferTop.getLevel() == firstLevel);

              points[segStart + ptOffsets[destBucket]]= bufferTop;
              ptOffsets[destBucket]++;

              if (ptOffsets[destBucket] < ptEnds[destBucket])
                bufferTop = points[segStart + ptOffsets[destBucket]];
              else
                bufferSize--;
            }
          }
        }
      }
    }
  }// end function()


  //
  // SFC_NodeSort::scanForDuplicates()
  //
  template <typename T, unsigned int dim>
  void SFC_NodeSort<T,dim>::scanForDuplicates(
      TNPoint<T,dim> *start, TNPoint<T,dim> *end,
      TNPoint<T,dim> * &firstCoarsest, TNPoint<T,dim> * &firstFinest,
      TNPoint<T,dim> * &next, unsigned int &numDups)
  {
    std::array<T,dim> first_coords, other_coords;
    start->getAnchor(first_coords);
    next = start + 1;
    firstCoarsest = start;
    firstFinest = start;
    unsigned char numInstances = start->get_numInstances();
    bool sameLevel = true;
    while (next < end && (next->getAnchor(other_coords), other_coords) == first_coords)
    {
      numInstances += next->get_numInstances();
      if (sameLevel && next->getLevel() != firstCoarsest->getLevel())
        sameLevel = false;
      if (next->getLevel() < firstCoarsest->getLevel())
        firstCoarsest = next;
      if (next->getLevel() > firstFinest->getLevel())
        firstFinest = next;
      next++;
    }
    if (sameLevel)
      numDups = numInstances;
    else
      numDups = 0;
  }


  template <typename T, unsigned int dim>
  struct ParentOfContainer
  {
    // PointType == TNPoint<T,dim>    KeyType == TreeNode<T,dim>
    TreeNode<T,dim> operator()(const TNPoint<T,dim> &pt) { return pt.getFinestOpenContainer(); }
  };


  //
  // SFC_NodeSort::countCGNodes_impl()
  //
  template <typename T, unsigned int dim>
  template<typename ResolverT>
  RankI SFC_NodeSort<T,dim>::countCGNodes_impl(
      ResolverT &resolveInterface,
      TNPoint<T,dim> *start, TNPoint<T,dim> *end,
      LevI sLev, RotI pRot,
      unsigned int order)
  {
    // Algorithm:
    //
    // Bucket points using the keyfun ParentOfContainer, and ancestors after sibilings.
    //   Points on our own interface end up in our `ancestor' bucket. We'll reconsider these later.
    //   Other points fall into the interiors of our children.
    // Recursively, for each child:
    //   Look up sfc rotation for child.
    //   Call countCGNodes_impl() on that child.
    //     The interfaces within all children are now resolved.
    // Reconsider our own interface.
    //   `Bucket' points on our interface, not by child, but by which hyperplane they reside on.
    //     Apply the test greedily, so that intersections have a consistent hyperplane choice.
    //   For each hyperplane:
    //     Sort the points (as points) in the SFC ordering, using ourselves as parent.
    //     Call resolveInterface() on the points in the hyperplane.

    using TNP = TNPoint<T,dim>;
    using TreeNode = TreeNode<T,dim>;
    constexpr char numChildren = TreeNode::numChildren;
    constexpr unsigned int rotOffset = 2*numChildren;  // num columns in rotations[].

    // Lookup tables to apply rotations.
    const ChildI * const rot_perm = &rotations[pRot*rotOffset + 0*numChildren];
    const RotI * const orientLookup = &HILBERT_TABLE[pRot*numChildren];

    if (!(start < end))
      return 0;

    RankI numUniqPoints = 0;

    // Reorder the buckets on sLev (current level).
    std::array<RankI, numChildren+1> tempSplitters;
    RankI ancStart, ancEnd;
    SFC_Tree<T,dim>::template SFC_bucketing_impl<ParentOfContainer<T,dim>, TNP, TreeNode>(
        start, 0, end-start, sLev, pRot,
        ParentOfContainer<T,dim>(), true, false,
        tempSplitters,
        ancStart, ancEnd);

    // Recurse.
    for (char child_sfc = 0; child_sfc < numChildren; child_sfc++)
    {
      // Check for empty bucket.
      if (tempSplitters[child_sfc+1] - tempSplitters[child_sfc+0] == 0)
        continue;

      ChildI child = rot_perm[child_sfc];
      RotI cRot = orientLookup[child];

      numUniqPoints += countCGNodes_impl<ResolverT>(
          resolveInterface,
          start + tempSplitters[child_sfc+0], start + tempSplitters[child_sfc+1],
          sLev+1, cRot,
          order);
    }

    // Process own interface. (In this case hlev == sLev).
    std::array<RankI, dim+1> hSplitters;
    bucketByHyperplane(start + ancStart, start + ancEnd, sLev, hSplitters);
    for (int d = 0; d < dim; d++)
    {
      locTreeSortAsPoints(
          start + ancStart, hSplitters[d], hSplitters[d+1],
          sLev, m_uiMaxDepth, pRot);

      // The actual counting happens here.
      numUniqPoints += resolveInterface(start + ancStart + hSplitters[d], start + ancStart + hSplitters[d+1], order);
    }

    return numUniqPoints;
  }


  //
  // SFC_NodeSort::bucketByHyperplane()
  //
  template <typename T, unsigned int dim>
  void SFC_NodeSort<T,dim>::bucketByHyperplane(TNPoint<T,dim> *start, TNPoint<T,dim> *end, unsigned int hlev, std::array<RankI,dim+1> &hSplitters)
  {
    // Compute offsets before moving points.
    std::array<RankI, dim> hCounts, hOffsets;
    hCounts.fill(0);
    for (TNPoint<T,dim> *pIter = start; pIter < end; pIter++)
      hCounts[pIter->get_firstIncidentHyperplane(hlev)]++;
    RankI accum = 0;
    for (int d = 0; d < dim; d++)
    {
      hOffsets[d] = accum;
      hSplitters[d] = accum;
      accum += hCounts[d];
    }
    hSplitters[dim] = accum;

    // Move points with a full size buffer.
    std::vector<TNPoint<T,dim>> buffer(end - start);
    for (TNPoint<T,dim> *pIter = start; pIter < end; pIter++)
      buffer[hOffsets[pIter->get_firstIncidentHyperplane(hlev)]++] = *pIter;
    for (auto &&pt : buffer)
      *(start++) = pt;
  }


  //
  // SFC_NodeSort::resolveInterface_lowOrder()
  //
  template <typename T, unsigned int dim>
  RankI SFC_NodeSort<T,dim>::resolveInterface_lowOrder(TNPoint<T,dim> *start, TNPoint<T,dim> *end, unsigned int order)
  {
    //
    // The low-order counting method is based on counting number of points per spatial location.
    // If there are points from two levels, then points from the higher level are non-hanging;
    // pick one of them as the representative (isSelected=Yes). The points from the lower
    // level are hanging and can be discarded (isSelected=No). If there are points all
    // from the same level, they are non-hanging iff we count pow(2,dim-cdim) instances, where
    // cdim is the dimension of the cell to which the points are interior. For example, in
    // a 4D domain, vertex-interior nodes (cdim==0) should come in sets of 16 instances per location,
    // while 3face-interior nodes (aka octant-interior nodes, cdim==3), should come in pairs
    // of instances per location.
    //
    // Given a spatial location, it is assumed that either all the instances generated for that location
    // are present on this processor, or none of them are.
    //
    using TNP = TNPoint<T,dim>;

    for (TNP *ptIter = start; ptIter < end; ptIter++)
      ptIter->set_isSelected(TNP::No);

    RankI totalCount = 0;
    TNP *ptIter = start;
    TNP *firstCoarsest, *unused_firstFinest;
    unsigned int numDups;
    while (ptIter < end)
    {
      scanForDuplicates(ptIter, end, firstCoarsest, unused_firstFinest, ptIter, numDups);
      if (!numDups)   // Signifies mixed levels. We have a winner.
      {
        firstCoarsest->set_isSelected(TNP::Yes);
        totalCount++;
      }
      else            // All same level and cell type. Test whether hanging or not.
      {
        unsigned char cdim = firstCoarsest->get_cellType().get_dim_flag();
        unsigned int expectedDups = 1u << (dim - cdim);
        if (numDups == expectedDups)
        {
          firstCoarsest->set_isSelected(TNP::Yes);
          totalCount++;
        }
      }
    }

    return totalCount;
  }


  //
  // SFC_NodeSort::resolveInterface_highOrder()
  //
  template <typename T, unsigned int dim>
  RankI SFC_NodeSort<T,dim>::resolveInterface_highOrder(TNPoint<T,dim> *start, TNPoint<T,dim> *end, unsigned int order)
  {
    //
    // The high-order counting method (order>=3) cannot assume that a winning node
    // will co-occur with finer level nodes in the same exact coordinate location.
    //
    // Instead, we have to consider open k-faces (k-cells), which generally
    // contain multiple nodal locations. We say an open k-cell is present when
    // an incident node of the same level is present. If an open k-cell is present,
    // it is non-hanging iff its parent open k'-cell is not present.
    //
    // # Locality property.
    // This algorithm relies on the `non-reentrant' property of SFC's visitng cells.
    // A corollary of non-reentrancy to K-cells (embedding dimension) is that,
    // once the SFC has entered a k-face of some level and orientation, it must
    // finish traversing the entire k-face before entering another k-face of
    // the same dimension and orientaton and of *coarser or equal level*.
    // The SFC is allowed to enter another k-face of finer level, but once
    // it has reached the finest level in a region, it cannot enter
    // a k-face of finer level. Therefore, considering a finer k-face in
    // a possibly mixed-level k-cell, the SFC length of the k-face is bounded
    // by a function of order and dimension only.
    //
    // # Overlap property.
    // This algorithm further relies on the order being 3 or higher.
    // Given an open k-cell C that is present, there are two cases.
    // - Case A: The parent P of the k-cell C has dimension k (C is `noncrossing').
    // - Case B: The parent P of the k-cell C has dimension k' > k (C is `crossing').
    // In Case A (C is noncrossing), if P is present, then at least one k-cell interior
    // node from P falls within the interior of C.
    // In Case B (C is crossing), then C is shared by k'-cell siblings under P;
    // if P is present, then, for each (k'-cell) child of P, at least one
    // k'-cell interior node from P falls within that child. Furthermore,
    // one of the k'-cell siblings under P shares the same K-cell anchor
    // as C.
    // --> This means we can detect the parent's node while traversing
    //     the child k-face, by checking levels of either k- or k'-cells.
    //
    // # Algorithm.
    // Combining the properties of SFC locality and high-order parent/child overlap,
    // we can determine for each k'-cell whether it is hanging or not, during a single
    // pass of the interface nodes by using a constant-sized buffer. (The buffer
    // size is a function of dimension and order, which for the current purpose
    // are constants).
    //
    // - Keep track of what K-cell we are traversing. Exiting the K-cell at any
    //   dimension means we must reset the rest of the state.
    //
    // - Keep track of which two distinct levels could be present on the k-faces
    //   of the current K-cell. There can be at most two distinct levels.
    //
    // - Maintain a table with one row per cell type (cell dimension, orientation).
    //   Each row is either uninitialized, or maintains the following:
    //   - the coarsest level of point hitherto witnessed for that cell type;
    //   - a list of pending points whose selection depends on the final coarseness of the k-face.
    //
    // - Iterate through all unique locations in the interface:
    //   - If the next point is contained in the current K-cell,
    //     - Find the point's native cell type, and update the coarseness of that cell type.
    //     - If the row becomes coarser while nodes are pending,
    //       - Deselect all pending nodes and remove them.
    //
    //     - If the two distinct levels in the K-cell are not yet known,
    //       and the new point does not differ in level from any seen before,
    //       - Append the point to a set of `unprocessed nodes'.
    //
    //     - Elif the two distinct levels in the K-cell are not yet known,
    //       and the new point differs in level from any seen before,
    //       - Put the new point on hold. Update the two distinct levels.
    //         Process all the unprocessed nodes (as defined next), then process the new point.
    //
    //     - Else, process the point:
    //       - If Lev(pt) == coarserLevel,
    //         - Set pt.isSelected := Yes.
    //       - Elif table[pt.parentCellType()].coarsestLevel == coarserLevel,
    //         - Set pt.isSelected := No.
    //       - Else
    //         - Append table.[pt.parentCellType()].pendingNodes <- pt
    //
    //   - Else, we are about to exit the current K-cell:
    //     - For all rows, select all pending points and remove them.
    //     - Reset the K-cell-wide state: Cell identity (anchor) and clear distinct levels.
    //     - Proceed with new point.
    //
    // - When we reach the end of the interface, select all remaining pending nodes.
    //

    using TNP = TNPoint<T,dim>;

    // Helper struct.
    struct kFaceStatus
    {
      void resetCoarseness()
      {
        m_isInitialized = false;
      }

      /** @brief Updates coarsenss to coarser or equal level. */
      void updateCoarseness(LevI lev)
      {
        if (!m_isInitialized || lev < m_curCoarseness)
        {
          m_curCoarseness = lev;
          m_isInitialized = true;
        }
      }

      RankI selectAndRemovePending()
      {
        RankI numSelected = m_pendingNodes.size();
        for (TNP *pt : m_pendingNodes)
          pt->set_isSelected(TNP::Yes);
        m_pendingNodes.clear();
        return numSelected;
      }

      void deselectAndRemovePending()
      {
        for (TNP *pt : m_pendingNodes)
        {
          pt->set_isSelected(TNP::No);
        }
        m_pendingNodes.clear();
      }

      // Data members.
      bool m_isInitialized = false;
      LevI m_curCoarseness;
      std::vector<TNP *> m_pendingNodes;
    };

    // Initialize table. //TODO make this table re-usable across function calls.
    int numLevelsWitnessed = 0;
    TreeNode<T,dim> currentKCell;
    LevI finerLevel;       // finerLevel and coarserLevel have meaning when
    LevI coarserLevel;     //   numLevelsWitnessed becomes 2, else see currentKCell.
    std::vector<kFaceStatus> statusTbl(1u << dim);   // Enough for all possible orientation indices.
    std::vector<TNP *> unprocessedNodes;

    RankI totalCount = 0;

    while (start < end)
    {
      // Get the next unique location (and cell type, using level as proxy).
      TNP *next = start + 1;
      while (next < end && (*next == *start))  // Compares both coordinates and level.
        (next++)->set_isSelected(TNP::No);
      start->set_isSelected(TNP::No);

      const unsigned char nCellType = start->get_cellType().get_orient_flag();
      kFaceStatus &nRow = statusTbl[nCellType];

      // First initialization of state for new K-cell.
      if (numLevelsWitnessed == 0 || !currentKCell.isAncestor(start->getDFD()))
      {
        // If only saw a single level, they are all non-hanging.
        totalCount += unprocessedNodes.size();
        for (TNP * &pt : unprocessedNodes)
          pt->set_isSelected(TNP::Yes);
        unprocessedNodes.clear();

        // We probably have some pending nodes to clean up.
        for (kFaceStatus &row : statusTbl)
        {
          totalCount += row.selectAndRemovePending();
          row.resetCoarseness();
        }

        // Initialize state to traverse new K-cell.
        currentKCell = start->getCell();
        numLevelsWitnessed = 1;
      }

      // Second initialization of state for new K-cell.
      if (numLevelsWitnessed == 1)
      {
        if (start->getLevel() < currentKCell.getLevel())
        {
          coarserLevel = start->getLevel();
          finerLevel = currentKCell.getLevel();
          numLevelsWitnessed = 2;
        }
        else if (start->getLevel() > currentKCell.getLevel())
        {
          coarserLevel = currentKCell.getLevel();
          finerLevel = start->getLevel();
          numLevelsWitnessed = 2;
          currentKCell = start->getCell();
        }
      }

      // Read from current node to update nRow.
      nRow.updateCoarseness(start->getLevel());
      if (numLevelsWitnessed == 2 && nRow.m_curCoarseness == coarserLevel)
        nRow.deselectAndRemovePending();

      // Enqueue node for later interaction with pRow.
      unprocessedNodes.push_back(start);

      // Process all recently added nodes.
      if (numLevelsWitnessed == 2)
      {
        for (TNP * &pt : unprocessedNodes)
        {
          const unsigned char pCellType = pt->get_cellTypeOnParent().get_orient_flag();
          kFaceStatus &pRow = statusTbl[pCellType];

          if (pt->getLevel() == coarserLevel)
          {
            pt->set_isSelected(TNP::Yes);
            totalCount++;
          }
          else if (pRow.m_isInitialized && pRow.m_curCoarseness == coarserLevel)
          {
            pt->set_isSelected(TNP::No);
          }
          else
          {
            pRow.m_pendingNodes.push_back(pt);
          }
        }
        unprocessedNodes.clear();
      }

      start = next;
    }

    // Finally, flush remaining unprocessed or pending nodes in table.
    totalCount += unprocessedNodes.size();
    for (TNP * &pt : unprocessedNodes)
      pt->set_isSelected(TNP::Yes);
    unprocessedNodes.clear();

    for (kFaceStatus &row : statusTbl)
      totalCount += row.selectAndRemovePending();

    return totalCount;
  }


  //
  // SFC_NodeSort::countInstances()
  //
  template <typename T, unsigned int dim>
  RankI SFC_NodeSort<T,dim>::countInstances(TNPoint<T,dim> *start, TNPoint<T,dim> *end, unsigned int unused_order)
  {
    using TNP = TNPoint<T,dim>;
    while (start < end)
    {
      // Get the next unique pair (location, level).
      unsigned char delta_numInstances = 0;
      TNP *next = start + 1;
      while (next < end && (*next == *start))  // Compares both coordinates and level.
      {
        delta_numInstances += next->get_numInstances();
        next->set_numInstances(0);
        next++;
      }
      start->incrementNumInstances(delta_numInstances);

      start = next;
    }

    return 0;
  }


  //
  // SFC_NodeSort::dist_bcastSplitters()
  //
  template <typename T, unsigned int dim>
  std::vector<TreeNode<T,dim>> SFC_NodeSort<T,dim>::dist_bcastSplitters(const TreeNode<T,dim> *start, MPI_Comm comm)
  {
    int nProc, rProc;
    MPI_Comm_rank(comm, &rProc);
    MPI_Comm_size(comm, &nProc);

    using TreeNode = TreeNode<T,dim>;
    std::vector<TreeNode> splitters(nProc);
    splitters[rProc] = *start;

    for (int turn = 0; turn < nProc; turn++)
      par::Mpi_Bcast<TreeNode>(&splitters[turn], 1, turn, comm);

    return splitters;
  }

  //
  // SFC_NodeSort::getProcNeighbours()
  //
  template <typename T, unsigned int dim>
  int SFC_NodeSort<T,dim>::getProcNeighbours(TNPoint<T,dim> pt,
      const TreeNode<T,dim> *splitters, int numSplitters,
      std::vector<int> &procNbList,
      unsigned int order)
  {
    std::vector<TreeNode<T,dim>> keyList(2*intPow(3,dim));    // Allocate then shrink. TODO static allocation
    keyList.clear();

    pt.getDFD().appendAllNeighboursAsPoints(keyList);  // Includes domain boundary points.

    // Fix to make sure we get 1-finer-level neighbours of the host k-face.
    // First check if the point is close to the center of the face.
    // Meanwhile work on constructing the center of the face, whose keys we may add too.
    const CellType<dim> ptCellType = pt.get_cellType();
    if (ptCellType.get_dim_flag() > 0)
    {
      bool appendCenterKeys = true;
      const unsigned int len = 1u << (m_uiMaxDepth - pt.getLevel());
      const T lenb2 = len >> 1u;
      const unsigned int faceOrientation = ptCellType.get_orient_flag();
      std::array<T,dim> elemCoords;
      pt.getCell().getAnchor(elemCoords);
      std::array<T,dim> faceCenterCoords = elemCoords;
      for (int d = 0; d < dim; d++)
      {
        if (faceOrientation & (1u << d))
        {
          long distFromCenter = (long) (pt.getX(d) - elemCoords[d]) - (long) lenb2;
          distFromCenter = (distFromCenter < 0 ? -distFromCenter : distFromCenter);
          if (!(distFromCenter * order < len))
          {
            appendCenterKeys = false;
            break;
          }
          faceCenterCoords[d] += lenb2;
        }
      }
      if (appendCenterKeys)
      {
        TreeNode<T,dim> centerPt(1, faceCenterCoords, m_uiMaxDepth);
        centerPt.appendAllNeighboursAsPoints(keyList);
      }
    }

    int procNbListSizeOld = procNbList.size();
    SFC_Tree<T,dim>::getContainingBlocks(keyList.data(), 0, (int) keyList.size(), splitters, numSplitters, procNbList);
    int procNbListSize = procNbList.size();

    return procNbListSize - procNbListSizeOld;
  }


  //
  // computeScattermap()
  //
  template <typename T, unsigned int dim>
  ScatterMap SFC_NodeSort<T,dim>::computeScattermap(const std::vector<TNPoint<T,dim>> &ownedNodes, const ScatterFacesCollection &scatterFaces)
  {
    SMVisit_data visitor_data;

    std::array<RankI, nSFOrient> sf_begin, sf_end;
    for (unsigned int ii = 0; ii < scatterFaces.size(); ii++)
    {
      sf_begin[ii] = 0;
      sf_end[ii] = (RankI) scatterFaces[ii].size();
    }

    // Adapters hold references to the visitor data. Different operator().
    SMVisit_count visitor_count(visitor_data);
    SMVisit_buildMap visitor_buildMap(visitor_data);

    // Counting pass.
    computeScattermap_impl<SMVisit_count>(
        ownedNodes, scatterFaces,
        0, (RankI) ownedNodes.size(),
        sf_begin, sf_end,
        0, m_uiMaxDepth, 0,  // Relies on special handling of the domain boundary.
        visitor_count);

    // Compute offsets to prep for the 2nd pass below.
    visitor_data.computeOffsets();

    /// // DEBUG TODO remove
    /// int rProc;
    /// MPI_Comm_rank(MPI_COMM_WORLD, &rProc);
    /// for (auto &&x : visitor_data.m_sendCountMap)
    /// {
    ///   fprintf(stderr, "[%d] -> (%d):  \t%u\n", rProc, x.first, x.second);
    /// }

    // Mapping pass.
    computeScattermap_impl<SMVisit_buildMap>(
        ownedNodes, scatterFaces,
        0, (RankI) ownedNodes.size(),
        sf_begin, sf_end,
        0, m_uiMaxDepth, 0,             // Relies on special handling of the domain boundary.
        visitor_buildMap);

    // Re-compute offsets after we advanced them in the 2nd pass above.
    visitor_data.computeOffsets();

    // Transfer mapping data to ScatterMap struct.
    int numProcSend = visitor_data.m_sendCountMap.size();

    ScatterMap sm;
    sm.m_map = visitor_data.m_scatterMap;
    sm.m_sendCounts.resize(numProcSend);
    sm.m_sendOffsets.resize(numProcSend);
    sm.m_sendProc.resize(numProcSend);

    auto countsIter = visitor_data.m_sendCountMap.cbegin();
    auto offsetsIter = visitor_data.m_sendOffsetsMap.cbegin();
    for (int procIdx = 0; procIdx < numProcSend; procIdx++, countsIter++, offsetsIter++)
    {
      sm.m_sendProc[procIdx] = countsIter->first;
      sm.m_sendCounts[procIdx] = countsIter->second;
      sm.m_sendOffsets[procIdx] = offsetsIter->second;
    }

    return sm;
  }


  template <typename T, unsigned int dim>
  struct GetCell
  {
    // PointType == TNPoint<T,dim>    KeyType == TreeNode<T,dim>
    TreeNode<T,dim> operator()(const TNPoint<T,dim> &pt) { return pt.getCell(); }
  };


  template <typename T, unsigned int dim>
  template <typename ActionT>
  void SFC_NodeSort<T,dim>::computeScattermap_impl(const std::vector<TNPoint<T,dim>> &ownedNodes,
      const SFC_NodeSort<T,dim>::ScatterFacesCollection &scatterFaces,
      RankI ownedNodes_bg, RankI ownedNodes_end,
      std::array<RankI, nSFOrient> scatterFaces_bg,
      std::array<RankI, nSFOrient> scatterFaces_end,
      LevI sLev, LevI eLev, RotI pRot,
      ActionT &visitAction)
  {
    //// Recursive Depth-first, similar to Most Significant Digit First. ////
    //// Adapted from locTreeSort.                                       ////

    using SF = ScatterFace<T,dim>;

    if (ownedNodes_end <= ownedNodes_bg) { return; }

    constexpr char numChildren = TreeNode<T,dim>::numChildren;
    constexpr unsigned int rotOffset = 2*numChildren;  // num columns in rotations[].

    // Locate buckets in ownedNodes. //
    std::array<RankI, numChildren+1> tempSplitters;
    RankI ancStart, ancEnd;
    SFC_Tree<T,dim>::template SFC_locateBuckets_impl<GetCell<T,dim>, TNPoint<T,dim>, TreeNode<T,dim>>(
        &(*ownedNodes.begin()), ownedNodes_bg, ownedNodes_end, sLev, pRot,
        GetCell<T,dim>(), true, true,
        tempSplitters,
        ancStart, ancEnd);

    // Locate buckets in each list of open kfaces. //
    std::array< std::array<RankI, nSFOrient>, numChildren+1> sfSplitters;
    std::array< RankI, nSFOrient> sfAncStart;
    std::array< RankI, nSFOrient> sfAncEnd;
    for (unsigned int orient = 0; orient < nSFOrient; orient++)
    {
      std::array<RankI, numChildren+1> getSFSplitters;
      SFC_Tree<T,dim>::template SFC_locateBuckets_impl<KeyFunIdentity_Pt<SF>, SF, SF>(
          &(*scatterFaces[orient].begin()), scatterFaces_bg[orient], scatterFaces_end[orient],
          sLev, pRot,
          KeyFunIdentity_Pt<SF>(), true, true,
          getSFSplitters, sfAncStart[orient], sfAncEnd[orient]);

      for (ChildI child_sfc = 0; child_sfc <= numChildren; child_sfc++)
        sfSplitters[child_sfc][orient] = getSFSplitters[child_sfc];
    }

    // Visit current node in SFC tree, if nonempty and has proc-bdry points.
    RankI numBdryPts = 0,  numNonBdryPts = 0;
    for (RankI ptIter = ancStart; ptIter < ancEnd; ptIter++)
    {
      if (ownedNodes[ptIter].get_owner() != -1) numBdryPts++;
      else                                      numNonBdryPts++;
    }
    if (numBdryPts > 0)
      visitAction(ownedNodes, scatterFaces, ancStart, ancEnd, sfAncStart, sfAncEnd);


    // Lookup tables to apply rotations.
    const ChildI * const rot_perm = &rotations[pRot*rotOffset + 0*numChildren];
    const RotI * const orientLookup = &HILBERT_TABLE[pRot*numChildren];

    if (sLev < eLev)  // This means eLev is further from the root level than sLev.
    {
      // Recurse.
      // Use the splitters to specify ranges for the next level of recursion.
      for (char child_sfc = 0; child_sfc < numChildren; child_sfc++)
      {
        ChildI child = rot_perm[child_sfc];
        RotI cRot = orientLookup[child];

        if (tempSplitters[child_sfc+1] - tempSplitters[child_sfc+0] < 1)
          continue;

        if (sLev > 0)
        {
          computeScattermap_impl(ownedNodes, scatterFaces,
              tempSplitters[child_sfc+0], tempSplitters[child_sfc+1],
              sfSplitters[child_sfc+0], sfSplitters[child_sfc+1],
              sLev+1, eLev, cRot,
              visitAction);
        }
        else   // Special handling if we have to consider the domain boundary.
        {
          computeScattermap_impl(ownedNodes, scatterFaces,
              tempSplitters[child_sfc+0], tempSplitters[child_sfc+1],
              sfSplitters[child_sfc+0], sfSplitters[child_sfc+1],
              sLev+1, eLev, pRot,
              visitAction);
        }
      }//for
    }//if
  }  // end function


  /**
   * @name visit_count
   * @brief Action of visitor for computeScattermap dual traversal, 1st pass counting.
   */
  template <typename T, unsigned int dim>
  void SFC_NodeSort<T,dim>::visit_count(SMVisit_data &visitor,
      const std::vector<TNPoint<T,dim>> &ownedNodes, const ScatterFacesCollection &scatterFaces,
      RankI ownedNodes_bg, RankI ownedNodes_end,
      const std::array<RankI, nSFOrient> &scatterFaces_bg,
      const std::array<RankI, nSFOrient> &scatterFaces_end)
  {
    for (RankI node_iter = ownedNodes_bg; node_iter < ownedNodes_end; node_iter++)
    {
      if (ownedNodes[node_iter].get_owner() != -1)
      {
        const unsigned int kfaceOrient = ownedNodes[node_iter].get_cellType().get_orient_flag();
        const RankI sf_bg = scatterFaces_bg[kfaceOrient];
        const RankI sf_end = scatterFaces_end[kfaceOrient];
        for (RankI sf_iter = sf_bg; sf_iter < sf_end; sf_iter++)
        {
          int neighbour = scatterFaces[kfaceOrient][sf_iter].get_owner();
          visitor.m_sendCountMap[neighbour]++;
        }
      }
    }
  }

  /**
   * @name visit_buildMap
   * @brief Action of visitor for computeScattermap dual traversal, 2nd pass mapping.
   */
  template <typename T, unsigned int dim>
  void SFC_NodeSort<T,dim>::visit_buildMap(SMVisit_data &visitor,
      const std::vector<TNPoint<T,dim>> &ownedNodes, const ScatterFacesCollection &scatterFaces,
      RankI ownedNodes_bg, RankI ownedNodes_end,
      const std::array<RankI, nSFOrient> &scatterFaces_bg,
      const std::array<RankI, nSFOrient> &scatterFaces_end)
  {
    for (RankI node_iter = ownedNodes_bg; node_iter < ownedNodes_end; node_iter++)
    {
      if (ownedNodes[node_iter].get_owner() != -1)
      {
        const unsigned int kfaceOrient = ownedNodes[node_iter].get_cellType().get_orient_flag();
        const RankI sf_bg = scatterFaces_bg[kfaceOrient];
        const RankI sf_end = scatterFaces_end[kfaceOrient];
        for (RankI sf_iter = sf_bg; sf_iter < sf_end; sf_iter++)
        {
          int neighbour = scatterFaces[kfaceOrient][sf_iter].get_owner();
          visitor.m_scatterMap[ visitor.m_sendOffsetsMap[neighbour]++ ] = node_iter;
        }
      }
    }
  }



  // ============================ End: SFC_NodeSort ============================ //

}//namespace ot
