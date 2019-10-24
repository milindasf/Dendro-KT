/**
 * @file:sfcTreeLoop.h
 * @author: Masado Ishii  --  UofU SoC,
 * @date: 2019-10-23
 * @brief: Stateful const iterator over implicit mesh, giving access to element nodes.
 *         Similar in spirit to eleTreeIterator.h, but hopefully an improvement.
 *
 *         My aim is to make the element loop more flexible and easier to reason about.
 */



/*
 * The recursive structure that is being mimicked:
 *
 * Traverse(subtree, parentData, nodeCoords[], input[], output[])
 * {
 *   // Vectors for children to read/write.
 *   stage_nodeCoords[NumChildren][];
 *   stage_input[NumChildren][];
 *   stage_output[NumChildren][];
 *   childSummaries[NumChildren];
 *
 *   parent2Child(parentData, subtree, nodeCoords, input, output);
 *
 *   UserPreAction(subtree, nodeCoords, input, output);
 *
 *   if (needToDescendFurther)
 *   {
 *     topDownNodes(subtree, nodeCoords,       input,       output,
 *                           stage_nodeCoords, stage_input, stage_output,
 *                           childSummaries);
 *
 *     for (child_sfc = 0; child_sfc < NumChildren; child_sfc++)
 *     {
 *       Traverse(subtree.getChildSFC(child_sfc),
 *                currentData,
 *                stage_nodeCoords[child_sfc],
 *                stage_input[child_sfc],
 *                stage_output[child_sfc]);
 *     }
 *
 *     bottomUpNodes(subtree, nodeCoords,       input,       output,
 *                            stage_nodeCoords, stage_input, stage_output,
 *                            childSummaries);
 *   }
 *
 *   UserPostAction(subtree, nodeCoords, input, output);
 *
 *   child2Parent(parentData, subtree, nodeCoords, input, output);
 * }
 *
 *
 * In the iterative version, the above function will be turned inside out.
 *
 *   - The call stack will be encapsulated inside a stateful iterator.
 *
 *   - The callbacks to UserPreAction() and UserPostAction() will be replaced
 *     by two pairs of entry-exit points whereupon program control is
 *     surrendered and regained at every level.
 *
 *   - The user can decide whether to descend or skip descending, at any level,
 *     by calling step() or next(), respectively.
 */


#ifndef DENDRO_KT_SFC_TREE_LOOP_H
#define DENDRO_KT_SFC_TREE_LOOP_H

#include "nsort.h"
#include "tsort.h"
#include "treeNode.h"
#include "mathUtils.h"
#include "binUtils.h"

/// #include "refel.h"
/// #include "tensor.h"


#include <vector>
#include <tuple>


namespace ot
{

  // ------------------------------
  // Class declarations
  // ------------------------------

  template <typename ...Types>
  class Inputs { };

  template <typename ...Types>
  class Outputs { };

  template <unsigned int dim, class InputTypes, class OutputTypes>
  class SFC_TreeLoop;

  template <unsigned int dim, class InputTypes, class OutputTypes>
  class SubtreeInfo;

  // Usage:
  //   E.g., for the matvec evaluation, subclass from
  //   - SFC_TreeLoop<dim, Inputs<TreeNode, double>, Outputs<double>>;
  //
  //   E.g., for matrix assembly counting, subclass from
  //   - SFC_TreeLoop<dim,
  //                  Inputs<TreeNode, DendroIntL>,
  //                  Outputs<TreeNode, TreeNode, DendroIntL, DendroIntL>>;
  //
  //   E.g., for matrix assembly evaluation, subclass from
  //   - SFC_TreeLoop<dim, Inputs<TreeNode, TreeNode>, Outputs<double>>

  //TODO if we want additional singleton values per frame (i.e. not vector),
  //can add another template parameter pack that defaults to empty....


  template <unsigned int dim, class InputTypes, class OutputTypes>
  class Frame;

  template <unsigned int dim, class InputTypes>
  struct FrameInputs {};        // Will define template specializations later.

  template <unsigned int dim, class OutputTypes>
  struct FrameOutputs {};       // Will define template specializations later.


  // ------------------------------
  // Class definitions
  // ------------------------------

  using ExtantCellFlagT = unsigned short;  // TODO use TreeNode extant cell flag type.

  //
  // SFC_TreeLoop
  //
  template <unsigned int dim, class InputTypes, class OutputTypes>
  class SFC_TreeLoop
  {
    static constexpr unsigned int NumChildren = 1u << dim;
    using C = unsigned int;
    using FrameT = Frame<dim, InputTypes, OutputTypes>;
    using SubtreeInfoT = SubtreeInfo<dim, InputTypes, OutputTypes>;

    friend SubtreeInfoT;

    protected:
      typename FrameInputs<dim, InputTypes>::DataStoreT   m_rootInputData;
      typename FrameOutputs<dim, OutputTypes>::DataStoreT m_rootOutputData;
      std::vector<FrameT> m_stack;

      // More stack-like things.

    public:
      // SFC_TreeLoop() : constructor
      SFC_TreeLoop()  //TODO
      {
        m_stack.emplace_back(m_rootInputData, m_rootOutputData);
      }

      // reset()
      void reset()
      {
        m_stack.clear();
        m_stack.emplace_back(m_rootInputData, m_rootOutputData);
      }

      // getSubtreeInfo()
      SubtreeInfoT getSubtreeInfo()  // Allowed to resize/write to myOutput in the top frame only.
      {
        return SubtreeInfoT(this);  //TODO
      }

      // step()
      bool step()
      {
        if (m_stack.back().m_isPre)
        {
          m_stack.reserve(m_stack.size() + NumChildren); // Avoid invalidating references.

          m_stack.back().m_isPre = false;
          FrameT &parentFrame = m_stack.back();
          parentFrame.m_extantChildren = 0u;   /*(1u << (1u << dim)) - 1;*/
          topDownNodes(parentFrame, &parentFrame.m_extantChildren);  // Free to resize children buffers.

          parentFrame.m_numExtantChildren = 0;

          // Push incident child frames in reverse order.
          for (ChildI child_sfc_rev = 0; child_sfc_rev < NumChildren; child_sfc_rev++)
          {
            const ChildI child_sfc = NumChildren-1 - child_sfc_rev;
            const RotI pRot = parentFrame.m_pRot;
            const ot::ChildI * const rot_perm = &rotations[pRot*2*NumChildren + 0*NumChildren];
            const ot::ChildI child_m = rot_perm[child_sfc];
            const ChildI cRot = HILBERT_TABLE[pRot*NumChildren + child_m];

            if (parentFrame.m_extantChildren & (1u << child_m))
            {
              m_stack.emplace_back(
                  &parentFrame,
                  child_sfc,
                  parentFrame.m_currentSubtree.getChildMorton(child_m),
                  cRot);
              parentFrame.m_numExtantChildren++;
            }
          }

          if (parentFrame.m_numExtantChildren > 0)
            // Enter the new top frame, which represents the 0th child.
            parent2Child(*m_stack.back().m_parentFrame, m_stack.back());
          else
            bottomUpNodes(parentFrame, parentFrame.m_extantChildren);

          return isPre();
        }
        else         // After a recursive call, can't step immediately.
          return next();
      }

      // next()
      bool next()
      {
        if (m_stack.size() > 1)
        {
          child2Parent(*m_stack.back().m_parentFrame, m_stack.back());
          m_stack.pop_back();
          // Return to the parent level.

          if (m_stack.back().m_isPre)
            // Enter the new top frame, which represents some other child.
            parent2Child(*m_stack.back().m_parentFrame, m_stack.back());
          else
            bottomUpNodes(m_stack.back(), m_stack.back().m_extantChildren);
        }
        else
          m_stack.back().m_isPre = false;

        return isPre();
      }

      // isPre()
      bool isPre()
      {
        return m_stack.back().m_isPre;
      }

      // isFinished()
      bool isFinished()
      {
        return (m_stack.size() == 1 && !m_stack.back().m_isPre);
      }



      // Must define
      /// virtual void topDownNodes(FrameT &parentFrame, ExtantCellFlagT *extantChildren);
      /// virtual void bottomUpNodes(FrameT &parentFrame, ExtantCellFlagT extantChildren);
      /// virtual void parent2Child(FrameT &parentFrame, FrameT &childFrame);
      /// virtual void child2Parent(FrameT &parentFrame, FrameT &childFrame);

      /**
       *  topDownNodes()
       *  is responsible to
       *    1. Resize the child input buffers in the parent frame;
       *
       *    2. Duplicate elements of the parent input buffers to
       *       incident child input buffers;
       *
       *    3. Indicate to SFC_TreeLoop which children to traverse,
       *       by accumulating into the extantChildren bit array.
       *
       *  Restrictions
       *    - MAY NOT resize or write to parent input buffers.
       *    - MAY NOT resize or write to variably sized output buffers.
       *
       *  Utilities are provided to identify and iterate over incident children.
       */
      virtual void topDownNodes(FrameT &parentFrame, ExtantCellFlagT *extantChildren) = 0;

      /**
       *  bottomUpNodes()
       *  is responsible to
       *    1. Resize the parent output buffers (handles to buffers are given);
       *
       *    2. Merge results from incident child output buffers into
       *       the parent output buffers.
       *
       *  The previously indicated extantChildren bit array will be supplied.
       *
       *  Utilities are provided to identify and iterate over incident children.
       */
      virtual void bottomUpNodes(FrameT &parentFrame, ExtantCellFlagT extantChildren) = 0;

      /**
       *  parent2Child()
       *  is responsible to
       *    1. Make available to the inspector any missing node data
       *       due to hanging nodes, e.g., by applying interpolation.
       */
      virtual void parent2Child(FrameT &parentFrame, FrameT &childFrame) = 0;

      /**
       *  child2Parent()
       *  is responsible to
       *    1. Propagate hanging node data (possibly modified by the inspector)
       *       back to parent nodes, e.g., by applying interpolation transpose.
       */
      virtual void child2Parent(FrameT &parentFrame, FrameT &childFrame) = 0;

    protected:
      const TreeNode<C,dim> & getCurrentSubtree()
      {
        assert (m_stack.size() > 0);
        return m_stack.back().m_currentSubtree;
      }

  };


  //
  // Frame
  //
  template <unsigned int dim, class InputTypes, class OutputTypes>
  class Frame
  {
    using C = unsigned int;
    static constexpr unsigned int NumChildren = 1u << dim;

    friend SFC_TreeLoop<dim, InputTypes, OutputTypes>;
    friend SubtreeInfo<dim, InputTypes, OutputTypes>;

    public:
      // Frame()
      Frame(typename FrameInputs<dim, InputTypes>::DataStoreT   &rootInputData,
            typename FrameOutputs<dim, OutputTypes>::DataStoreT &rootOutputData)
        : i(rootInputData),
          o(rootOutputData),
          m_parentFrame(nullptr),
          m_currentSubtree()
      {
        m_pRot = 0;
        m_isPre = true;
        m_extantChildren = 0u;
        m_numExtantChildren = 0;
      }

      // Frame()
      Frame(Frame *parentFrame, ChildI child, TreeNode<C,dim> &&subtree, RotI pRot)
        : i(parentFrame->i.childData[child]),
          o(parentFrame->o.childData[child]),
          m_parentFrame(parentFrame),
          m_currentSubtree(subtree),
          m_pRot(pRot)
      {
        m_isPre = true;
        m_extantChildren = 0u;
        m_numExtantChildren = 0;
      }

      // Frame()
      Frame(Frame &&) = default;

      // Frame()
      Frame(const Frame &) = delete;

      // getMyInputHandle<>()
      template <unsigned int inputIdx>
      typename std::tuple_element<inputIdx, typename FrameInputs<dim, InputTypes>::DataStoreT>::type
          & getMyInputHandle() { return std::get<inputIdx>(i.myDataHandles); }

      // getMyOutputHandle<>()
      template <unsigned int outputIdx>
      typename std::tuple_element<outputIdx, typename FrameOutputs<dim, OutputTypes>::DataStoreT>::type
          & getMyOutputHandle() { return std::get<outputIdx>(o.myDataHandles); }

      // getChildInput<>()
      template <unsigned int inputIdx>
      typename std::tuple_element<inputIdx, typename FrameInputs<dim, InputTypes>::DataStoreT>::type
          & getChildInput(ChildI ch) { return std::get<inputIdx>(i.childData[ch]); }

      // getChildOutput<>()
      template <unsigned int outputIdx>
      typename std::tuple_element<outputIdx, typename FrameOutputs<dim, OutputTypes>::DataStoreT>::type
          & getChildOutput(ChildI ch) { return std::get<outputIdx>(o.childData[ch]); }

    public:
      FrameInputs<dim, InputTypes> i;
      FrameOutputs<dim, OutputTypes> o;

    private:
      Frame *m_parentFrame;
      TreeNode<C,dim> m_currentSubtree;
      bool m_isPre;
      RotI m_pRot;
      unsigned int m_numExtantChildren;
      ExtantCellFlagT m_extantChildren;
  };


  //
  // FrameInputs
  //
  template <unsigned int dim, typename ...Types>
  struct FrameInputs<dim, Inputs<Types...>>
  {
    using DataStoreT = std::tuple<std::vector<Types> ...>;

    //TODO replace the separate std::vector by TopDownStackSlice
    public:
      FrameInputs(std::tuple<std::vector<Types> ...> &myDataStore)
        : myDataHandles(myDataStore) { };
      FrameInputs(FrameInputs &&) = default;
      FrameInputs(const FrameInputs &) = delete;

      // Originally wanted tuple of references but I guess a reference to tuple will have to do.
      std::tuple<std::vector<Types>  ...> &myDataHandles;
      std::tuple<std::vector<Types> ...> childData[1u << dim];
  };


  //
  // FrameOutputs
  //
  template <unsigned int dim, typename ...Types>
  struct FrameOutputs<dim, Outputs<Types...>>
  {
    using DataStoreT = std::tuple<std::vector<Types> ...>;

    //TODO replace the separate std::vector by BottomUpStackSlice
    public:
      FrameOutputs(std::tuple<std::vector<Types> ...> &myDataStore)
        : myDataHandles(myDataStore) { };
      FrameOutputs(FrameOutputs &&) = default;
      FrameOutputs(const FrameOutputs &) = delete;

      // Originally wanted tuple of references but I guess a reference to tuple will have to do.
      std::tuple<std::vector<Types>  ...> &myDataHandles;
      std::tuple<std::vector<Types> ...> childData[1u << dim];
  };


  //
  // SubtreeInfo
  //
  template <unsigned int dim, class InputTypes, class OutputTypes>
  class SubtreeInfo
  {
    public:
      SubtreeInfo(SFC_TreeLoop<dim, InputTypes, OutputTypes> *treeLoopPtr)
      {
        m_treeLoopPtr = treeLoopPtr;
        //TODO
      }

      const TreeNode<unsigned int,dim> & getCurrentSubtree()
      {
        assert(m_treeLoopPtr != nullptr);
        return m_treeLoopPtr->m_stack.back().m_currentSubtree;
      }

    private:
      SFC_TreeLoop<dim, InputTypes, OutputTypes> *m_treeLoopPtr;

  };


}




#endif//DENDRO_KT_SFC_TREE_LOOP_H
