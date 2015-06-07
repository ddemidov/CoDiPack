/**
 * CoDiPack, a Code Differentiation Package
 *
 * Copyright (C) 2015 Chair for Scientific Computing, TU Kaiserslautern
 *
 * This file is part of CoDiPack.
 *
 * CoDiPack is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version 2 of the
 * License, or (at your option) any later version.
 *
 * CoDiPack is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details.
 * You should have received a copy of the GNU
 * General Public License along with CoDiPack.
 * If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: TODO
 */

#pragma once

#include <iostream>
#include <cstddef>
#include <tuple>

#include "../activeReal.hpp"
#include "chunk.hpp"
#include "chunkVector.hpp"
#include "reverseTapeInterface.hpp"
#include "externalFunctions.hpp"

namespace codi {

  /**
   * @brief Terminator for a sequence of ChunkVectors, which counts the number of statements.
   *
   * The structure is a helper structure for the ChunkTape. It terminates the ChunkVector sequence
   * and counts the number of expressions which have been recorded on the tape.
   *
   * @tparam IndexType The type of the index for the counting.
   */
  template <typename IndexType>
  struct ExpressionCounter {

    /**
     * @brief The needed position definition for a ChunkVector sequence terminator.
     *
     * Just the integer for the current statement.
     */
    typedef IndexType Position;

    /**
     * @brief The current count for the statements.
     */
    IndexType count;

    /**
     * @brief The needed getPosition method for a ChunkVector sequence terminator.
     *
     * The method returns the current state of the count value.
     * @return The current value of count.
     */
    inline Position getPosition() {
      return count;
    }

    /**
     * @brief The needed reset method for a ChunkVector sequence terminator.
     *
     * The method sets count to the value from the argument.
     *
     * @param pos The new value of count.
     */
    inline void reset(const Position& pos) {
      count = pos;
    }
  };

  /**
   * @brief Helper struct to define the nested chunk vectors for the ChunkTape.
   *
   * See #ChunkTape for details.
   */
  template <typename Real, typename IndexType>
  struct ChunkTapeTypes {
    /** @brief The data for the jacobies of each statement */
    typedef Chunk2< Real, IndexType> DataChunk;
    /** @brief The chunk vector for the jacobi data. */
    typedef ChunkVector<DataChunk, ExpressionCounter<IndexType> > DataChunkVector;

    /** @brief The data for each statement. */
    typedef Chunk1<StatementInt> StatementChunk;
    /** @brief The chunk vector for the statement data. */
    typedef ChunkVector<StatementChunk, DataChunkVector> StatementChunkVector;

    /** @brief The data for the external functions. */
    typedef Chunk2<ExternalFunction,typename StatementChunkVector::Position> ExternalFunctionChunk;
    /** @brief The chunk vector for the external  function data. */
    typedef ChunkVector<ExternalFunctionChunk, StatementChunkVector> ExternalFunctionChunkVector;

    /** @brief The position for all the different data vectors. */
    typedef typename ExternalFunctionChunkVector::Position Position;

  };

  /**
   * @brief A tape which grows if more space is needed.
   *
   * The ChunkTape implements a fully featured ReverseTapeInterface in a most
   * user friendly fashion. The storage vectors of the tape are grown if the
   * tape runs out of space.
   *
   * This is handled by a nested definition of multiple ChunkVectors which hold
   * the different data vectors. The current implementation uses 3 ChunkVectors
   * and one terminator. The relation is
   *
   * externalFunctions -> statements -> jacobiData -> expressionCounter.
   *
   * The size of the tape can be set with the resize function,
   * the tape will allocate enough chunks such that the given data requirements will fit into the chunks.
   *
   * @tparam      Real  The floating point type used in the ActiveReal.
   * @tparam IndexType  The type for the indexing of the adjoint variables.
   */
  template <typename Real, typename IndexType>
  class ChunkTape : public ReverseTapeInterface<Real, IndexType, ChunkTape<Real, IndexType>, typename ChunkTapeTypes<Real, IndexType>::Position > {
  public:

    /** @brief The data for the jacobies of each statement */
    typedef typename ChunkTapeTypes<Real, IndexType>::DataChunk DataChunk;
    /** @brief The chunk vector for the jacobi data. */
    typedef typename ChunkTapeTypes<Real, IndexType>::DataChunkVector DataChunkVector;

    /** @brief The data for each statement. */
    typedef typename ChunkTapeTypes<Real, IndexType>::StatementChunk StatementChunk;
    /** @brief The chunk vector for the statement data. */
    typedef typename ChunkTapeTypes<Real, IndexType>::StatementChunkVector StatementChunkVector;

    /** @brief The data for the external functions. */
    typedef typename ChunkTapeTypes<Real, IndexType>::ExternalFunctionChunk ExternalFunctionChunk;
    /** @brief The chunk vector for the external  function data. */
    typedef typename ChunkTapeTypes<Real, IndexType>::ExternalFunctionChunkVector ExternalFunctionChunkVector;

    /** @brief The position for all the different data vectors. */
    typedef typename ChunkTapeTypes<Real, IndexType>::Position Position;


  private:

    /** @brief The counter for the current expression. */
    ExpressionCounter<IndexType> expressionCount;
    /** @brief The data for the jacobies of each statements. */
    DataChunkVector data;
    /** @brief The data for the statements. */
    StatementChunkVector statements;
    /** @brief The data for the external functions. */
    ExternalFunctionChunkVector externalFunctions;

    /**
     * @brief The adjoint vector.
     *
     * The size of the adjoint vector is set according to the requested positions.
     * But the positions should not be greater than the current expression counter.
     */
    Real* adjoints;
    /* @brief The current size of the adjoint vector. */
    IndexType adjointsSize;

    /**
     * @brief Determines if statements are recorded or ignored.
     */
    bool active;

  public:
    /**
     * @brief Creates a tape with the default chunk sizes for the data, statements and
     * external functions defined in the configuration.
     */
    ChunkTape() :
      expressionCount(),
      data(DefaultChunkSize, expressionCount),
      statements(DefaultChunkSize, data),
      externalFunctions(1000, statements),
      adjoints(NULL),
      adjointsSize(0),
      active(false){
    }

    /**
     * @brief Set the size of the jacobi data chunks.
     *
     * @param[in] dataChunkSize The new size for the jacobi data chunks.
     */
    void setDataChunkSize(const size_t& dataChunkSize) {
      data.setChunkSize(dataChunkSize);
    }

    /**
     * @brief Set the size of the statement data chunks.
     *
     * @param[in] statementChunkSize The new size for the statement data chunks.
     */
    void setStatementChunkSize(const size_t& statementChunkSize) {
      statements.setChunkSize(statementChunkSize);
    }

    /**
     * @brief Set the size of the external function data chunks.
     *
     * @param[in] extChunkSize The new size for the external function data chunks.
     */
    void setExternalFunctionChunkSize(const size_t& extChunkSize) {
      externalFunctions.setChunkSize(extChunkSize);
    }

    /**
     * @brief Set the size of the jacobi and statement data.
     *
     * The tape will allocate enough chunks such that the given data
     * sizes will fit into the chunk vectors.
     *
     * @param[in]      dataSize  The new size of the jacobi data.
     * @param[in] statementSize  The new size of the statement data.
     */
    void resize(const size_t& dataSize, const size_t& statementSize) {
      data.resize(dataSize);
      statements.resize(statementSize);
    }

private:
    /**
     * @brief Helper function: Sets the adjoint vector to a new size.
     *
     * @param[in] size The new size for the adjoint vector.
     */
    void resizeAdjoints(const IndexType& size) {
      IndexType oldSize = adjointsSize;
      adjointsSize = size;

      adjoints = (Real*)realloc(adjoints, sizeof(Real) * (size_t)adjointsSize);

      for(IndexType i = oldSize; i < adjointsSize; ++i) {
        adjoints[i] = 0.0;
      }
    }

public:
    /**
     * @brief Allocate the adjoint vector such that it fits the current number of statements.
     */
    void allocateAdjoints() {
      //TODO: Tim fragen of er das brauch
      resizeAdjoints(expressionCount + 1);
    }

    /**
     * @brief Store the jacobies of the statement on the tape.
     *
     * The jacobies and the indices of the rhs expression are
     * stored on the tape. Also the number of active variables
     * is stored in the statement vector.
     *
     * The gradient data of the lhs will get a new index.
     * The primal value of the lhs is set to the primal value of the rhs.
     *
     * @param[out]   lhsValue    The primal value of the lhs. This value is set to the value
     *                           of the right hand side.
     * @param[out]   lhsIndex    The gradient data of the lhs. The index will be updated.
     * @param[in]         rhs    The right hand side expression of the assignment.
     *
     * @tparam Rhs The expression on the rhs of the statement.
     */
    template<typename Rhs>
    inline void store(Real& lhsValue, IndexType& lhsIndex, const Rhs& rhs) {
      Real gradient; /* This value will not be used */

      ENABLE_CHECK (OptTapeActivity, active){
        data.reserveItems(ExpressionTraits<Rhs>::maxActiveVariables);
        statements.reserveItems(1); // statements needs a reserve before the data items for the statement are pushed
        /* first store the size of the current stack position and evaluate the
         rhs expression. If there was an active variable on the rhs, update
         the index of the lhs */
        size_t startSize = data.getChunkPosition();
        rhs.calcGradient(gradient);
        size_t activeVariables = data.getChunkPosition() - startSize;
        if(0 == activeVariables) {
          lhsIndex = 0;
        } else {
          statements.setDataAndMove(std::make_tuple((StatementInt)activeVariables));
          lhsIndex = ++expressionCount.count;
        }
        // TODO: Hack aufheben oder schaltbar machen?
//      } else {
//        lhsIndex = 0;
      }
      /* now set the value of the lhs */
      lhsValue = rhs.getValue();
    }

    /**
     * @brief Optimization for the copy operation just copies the index of the rhs.
     *
     * No data is stored in this method.
     *
     * The primal value of the lhs is set to the primal value of the rhs.
     *
     * @param[out]   lhsValue    The primal value of the lhs. This value is set to the value
     *                           of the right hand side.
     * @param[out]   lhsIndex    The gradient data of the lhs. The index will be set to the index of the rhs.
     * @param[in]         rhs    The right hand side expression of the assignment.
     */
    inline void store(Real& lhsValue, IndexType& lhsIndex, const ActiveReal<Real, ChunkTape<Real, IndexType> >& rhs) {
      ENABLE_CHECK (OptTapeActivity, active){
        lhsIndex = rhs.getGradientData();
        // TODO: Hack aufheben oder schaltbar machen?
//      } else {
//        lhsIndex = 0;
      }
      lhsValue = rhs.getValue();
    }

    /**
     * @brief Optimization for a passive value on the rhs. The lhs index is set to zero.
     *
     * No data is stored in this method.
     *
     * The primal value of the lhs is set to the primal value of the rhs.
     *
     * @param[out]   lhsValue    The primal value of the lhs. This value is set to the value
     *                           of the right hand side.
     * @param[out]   lhsIndex    The gradient data of the lhs. The index will be set to zero.
     * @param[in]         rhs    The right hand side expression of the assignment.
     */
    inline void store(Real& lhsValue, IndexType& lhsIndex, const typename TypeTraits<Real>::PassiveReal& rhs) {
      ENABLE_CHECK (OptTapeActivity, active){
        lhsIndex = 0;
      }
      lhsValue = rhs;
    }

    /**
     * @brief Stores the jacobi with the value 1.0 on the tape if the index is active.
     *
     * @param[in] gradient Not used in this implementation.
     * @param[in]    value Not used in this implementation.
     * @param[in]    index Used to check if the variable is active.
     */
    inline void pushJacobi(Real& gradient, const Real& value, const IndexType& index) {
      CODI_UNUSED(gradient);
      CODI_UNUSED(value);
      if(0 != index) {
        data.setDataAndMove(std::make_tuple(1.0, index));
      }
    }

    /**
     * @brief Stores the jacobi on the tape if the index is active.
     *
     * @param[in] gradient Not used in this implementation.
     * @param[in]   jacobi Stored on the tape if the variable is active.
     * @param[in]    value Not used in this implementation.
     * @param[in]    index Used to check if the variable is active.
     */
    inline void pushJacobi(Real& gradient, const Real& jacobi, const Real& value, const IndexType& index) {
      CODI_UNUSED(gradient);
      CODI_UNUSED(value);
      if(0 != index) {
        ENABLE_CHECK(OptIgnoreInvalidJacobies, isfinite(jacobi)) {
          ENABLE_CHECK(OptJacobiIsZero, 0.0 != jacobi) {
            data.setDataAndMove(std::make_tuple(jacobi, index));
          }
        }
      }
    }

    /**
     * @brief Set the index to zero.
     * @param[in] value Not used in this implementation.
     * @param[out] index The index of the active type.
     */
    inline void initGradientData(Real& value, IndexType& index) {
      CODI_UNUSED(value);
      index = 0;
    }

    /**
     * @brief Does nothing.
     * @param[in] value Not used in this implementation.
     * @param[in] index Not used in this implementation.
     */
    inline void destroyGradientData(Real& value, IndexType& index) {
      CODI_UNUSED(value);
      CODI_UNUSED(index);
      /* nothing to do */
    }


    /**
     * @brief Set the gradient value of the corresponding index.
     *
     * If the index 0 is the inactive indicator and is ignored.
     *
     * @param[in]    index  The index of the active type.
     * @param[in] gradient  The new value for the gradient.
     */
    void setGradient(IndexType& index, const Real& gradient) {
      if(0 != index) {
        this->gradient(index) = gradient;
      }
    }

    /**
     * @brief Get the gradient value of the corresponding index.
     *
     * @param[in] index The index of the active type.
     * @return The gradient value corresponding to the given index.
     */
    inline Real getGradient(const IndexType& index) const {
      if(adjointsSize <= index) {
        return Real();
      } else {
        return adjoints[index];
      }
    }

    /**
     * @brief Get a reference to the gradient value of the corresponding index.
     *
     * An index of 0 will raise an assert exception.
     *
     * @param[in] index The index of the active type.
     * @return The reference to the gradient data.
     */
    inline Real& gradient(IndexType& index) {
      assert(0 != index);

      //TODO: Add error when index is bigger than expression count
      if(adjointsSize <= index) {
        resizeAdjoints(index + 1);
      }

      return adjoints[index];
    }

    /**
     * @brief Get the current position of the tape.
     *
     * The position can be used to reset the tape to that position or to
     * evaluate only parts of the tape.
     * @return The current position of the tape.
     */
    inline Position getPosition() {
      return externalFunctions.getPosition();
    }

    /**
     * @brief Sets all adjoint/gradients to zero.
     */
    inline void clearAdjoints(){
      for(IndexType i = 0; i <= expressionCount.count; ++i) {
        adjoints[i] = 0.0;
      }
    }

    /**
     * @brief Sets all adjoint/gradients to zero.
     */
    inline void clearAdjoints(const Position& start, const Position& end){
      for(IndexType i = start.inner.inner.inner; i <= end.inner.inner.inner; ++i) {
        adjoints[i] = 0.0;
      }
    }

    /**
     * @brief Reset the tape to the given position.
     *
     * @param[in] pos Reset the state of the tape to the given position.
     */
    inline void reset(const Position& pos) {
      for(IndexType i = pos.inner.inner.inner; i <= expressionCount.count; ++i) {
        adjoints[i] = 0.0;
      }

      externalFunctions.forEach(externalFunctions.getPosition(), pos, popExternalFunction);

      // reset will be done iteratively through the vectors
      externalFunctions.reset(pos);
    }

    /**
     * @brief Reset the tape to its initial state.
     */
    inline void reset() {
      reset(Position());
    }

  private:

    /**
     * @brief Implementation of the AD stack evaluation.
     *
     * It has to hold startAdjPos >= endAdjPos.
     *
     * @param[in] startAdjPos The starting point in the expression evaluation.
     * @param[in]   endAdjPos The ending point in the expression evaluation.
     * @param[inout]  stmtPos The current position in the statement vector. This value is used in the next invocation of this method.
     * @param[in]  statements The pointer to the statement vector.
     * @param[inout]  dataPos The current position in the jacobi and index vector. This value is used in the next invocation of this method..
     * @param[in]    jacobies The pointer to the jacobi vector.
     * @param[in]     indices The pointer to the index vector
     */
    inline void evaluateExpressions(const size_t& startAdjPos, const size_t& endAdjPos, size_t& stmtPos, StatementInt* &statements, size_t& dataPos, Real* &jacobies, IndexType* &indices) {
      size_t adjPos = startAdjPos;

      while(adjPos > endAdjPos) {
        const Real& adj = adjoints[adjPos];
        --adjPos;
        --stmtPos;
        const StatementInt& activeVariables = statements[stmtPos];
        ENABLE_CHECK(OptZeroAdjoint, adj != 0){
          for(StatementInt curVar = 0; curVar < activeVariables; ++curVar) {
            --dataPos;
            adjoints[indices[dataPos]] += adj * jacobies[dataPos];

          }
        } else {
          dataPos -= activeVariables;
        }
      }
    }

    /**
     * @brief Evaluate a part of the statement vector.
     *
     * It has to hold start >= end.
     *
     * The function calls the evaluation method for the jacobi vector.
     *
     * @param[in] start The starting point for the statement vector.
     * @param[in]   end The ending point for the statement vector.
     */
    inline void evaluateStmt(const typename StatementChunkVector::Position& start, const typename StatementChunkVector::Position& end) {
      StatementInt* statementData;
      size_t dataPos = start.data;
      typename DataChunkVector::Position curInnerPos = start.inner;
      for(size_t curChunk = start.chunk; curChunk > end.chunk; --curChunk) {
        std::tie(statementData) = statements.getDataAtPosition(curChunk, 0);

        typename DataChunkVector::Position endInnerPos = statements.getInnerPosition(curChunk);
        evaluateData(curInnerPos, endInnerPos, dataPos, statementData);

        curInnerPos = endInnerPos;

        dataPos = statements.getChunkUsedData(curChunk - 1);
      }

      // Iterate over the reminder also covers the case if the start chunk and end chunk are the same
      std::tie(statementData) = statements.getDataAtPosition(end.chunk, 0);
      evaluateData(curInnerPos, end.inner, dataPos, statementData);
    }

    /**
     * @brief Evaluate a part of the jacobi vector.
     *
     * It has to hold start >= end.
     *
     * It calls the evaluation method for the expression counter.
     *
     * @param[in]       start The starting point for the jacobi vector.
     * @param[in]         end The ending point for the jacobi vector.
     * @param[inout]  stmtPos The current position in the statement vector. This value is used in the next invocation of this method.
     * @param[in]  statements The pointer to the statement vector.
     */
    inline void evaluateData(const typename DataChunkVector::Position& start, const typename DataChunkVector::Position& end, size_t& stmtPos, StatementInt* &statementData) {
      Real* jacobiData;
      IndexType* indexData;
      size_t dataPos = start.data;
      typename ExpressionCounter<IndexType>::Position curInnerPos = start.inner;
      for(size_t curChunk = start.chunk; curChunk > end.chunk; --curChunk) {
        std::tie(jacobiData, indexData) = data.getDataAtPosition(curChunk, 0);

        typename ExpressionCounter<IndexType>::Position endInnerPos = data.getInnerPosition(curChunk);
        evaluateExpressions(curInnerPos, endInnerPos, stmtPos, statementData, dataPos, jacobiData, indexData);

        curInnerPos = endInnerPos;

        dataPos = data.getChunkUsedData(curChunk - 1);
      }

      // Iterate over the reminder also covers the case if the start chunk and end chunk are the same
      std::tie(jacobiData, indexData) = data.getDataAtPosition(end.chunk, 0);
      evaluateExpressions(curInnerPos, end.inner, stmtPos, statementData, dataPos, jacobiData, indexData);
    }

    /*
     * Function object for the evaluation of the external functions.
     *
     * It stores the last position for the statement vector. With this
     * position it evaluates the statement vector to the position
     * where the external function was added and then calls the
     * external function.
     */
    struct ExtFuncEvaluator {
      typename StatementChunkVector::Position curInnerPos;
      ExternalFunction* extFunc;
      typename StatementChunkVector::Position* endInnerPos;

      ChunkTape<Real, IndexType>& tape;

      ExtFuncEvaluator(typename StatementChunkVector::Position curInnerPos, ChunkTape<Real, IndexType>& tape) :
        curInnerPos(curInnerPos),
        extFunc(NULL),
        endInnerPos(NULL),
        tape(tape){}

      void operator () (typename ExternalFunctionChunk::DataPointer& data) {
        std::tie(extFunc, endInnerPos) = data;

        // always evaluate the stack to the point of the external function
        tape.evaluateStmt(curInnerPos, *endInnerPos);

        extFunc->evaluate();

        curInnerPos = *endInnerPos;
      }
    };

    /**
     * @brief Evaluate a part of the external function vector.
     *
     * It has to hold start >= end.
     *
     * It calls the evaluation method for the statement vector.
     *
     * @param[in]       start The starting point for the external function vector.
     * @param[in]         end The ending point for the external function vector.
     */
    void evaluateExtFunc(const typename ExternalFunctionChunkVector::Position& start, const typename ExternalFunctionChunkVector::Position &end){
      ExtFuncEvaluator evaluator(start.inner, *this);

      externalFunctions.forEach(start, end, evaluator);

      // Iterate over the reminder also covers the case if there have been no external functions.
      evaluateStmt(evaluator.curInnerPos, end.inner);
    }

  public:
    /**
     * @brief Perform the adjoint evaluation from start to end.
     *
     * It has to hold start >= end.
     *
     * @param[in] start  The starting position for the adjoint evaluation.
     * @param[in]   end  The ending position for the adjoint evaluation.
     */
    void evaluate(const Position& start, const Position& end) {
      if(adjointsSize <= expressionCount.count) {
        resizeAdjoints(expressionCount.count + 1);
      }

      evaluateExtFunc(start, end);
    }


    /**
     * @brief Perform the adjoint evaluation from the current position to the initial position.
     */
    void evaluate() {
      evaluate(getPosition(), Position());
    }

    /**
     * @brief Add an external function with a void handle as user data.
     *
     * The data handle provided to the tape is considered in possession of the tape. The tape will now be responsible to
     * free the handle. For this it will use the delete function provided by the user.
     *
     * @param[in] extFunc  The external function which is called by the tape.
     * @param[inout] data  The data for the external function. The tape takes ownership over the data.
     * @param[in] delData  The delete function for the data.
     */
    void pushExternalFunctionHandle(ExternalFunction::CallFunction extFunc, void* data, ExternalFunction::DeleteFunction delData){
      pushExternalFunctionHandle(ExternalFunction(extFunc, data, delData));
    }


    /**
     * @brief Add an external function with a specific data type.
     *
     * The data pointer provided to the tape is considered in possession of the tape. The tape will now be responsible to
     * free the data. For this it will use the delete function provided by the user.
     *
     * @param[in] extFunc  The external function which is called by the tape.
     * @param[inout] data  The data for the external function. The tape takes ownership over the data.
     * @param[in] delData  The delete function for the data.
     */
    template<typename Data>
    void pushExternalFunction(typename ExternalFunctionDataHelper<Data>::CallFunction extFunc, Data* data, typename ExternalFunctionDataHelper<Data>::DeleteFunction delData){
      pushExternalFunctionHandle(ExternalFunctionDataHelper<Data>::createHandle(extFunc, data, delData));\
    }

  private:
    /**
     * @brief Private common method to add to the external function stack.
     *
     * @param[in] function The external function structure to push.
     */
    void pushExternalFunctionHandle(const ExternalFunction& function){
      externalFunctions.reserveItems(1);
      externalFunctions.setDataAndMove(std::make_tuple(function, statements.getPosition()));
    }

    /**
     * @brief Delete the data of the external function.
     * @param extFunction The external function in the vector.
     */
    static void popExternalFunction(typename ExternalFunctionChunk::DataPointer& extFunction) {
      /* we just need to call the delete function */
      std::get<0>(extFunction)->deleteData();
    }
  public:

    inline void registerInput(ActiveReal<Real, ChunkTape<Real, IndexType> >& value) {
      statements.reserveItems(1);
      statements.setDataAndMove(std::make_tuple(0));

      value.getGradientData() = ++expressionCount.count;
    }

    inline void registerOutput(ActiveReal<Real, ChunkTape<Real, IndexType> >& value) {
      CODI_UNUSED(value);
      /* do nothing */
    }

    /**
     * @brief Start recording.
     */
    inline void setActive(){
      active = true;
    }

    /**
     * @brief Stop recording.
     */
    inline void setPassive(){
      active = false;
    }

    /**
     * @brief Check if the tape is active.
     * @return true if the tape is active.
     */
    inline bool isActive(){
      return active;
    }

  };
}
