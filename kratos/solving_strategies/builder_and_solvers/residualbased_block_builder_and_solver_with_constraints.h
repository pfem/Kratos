//    |  /           |
//    ' /   __| _` | __|  _ \   __|
//    . \  |   (   | |   (   |\__ `
//   _|\_\_|  \__,_|\__|\___/ ____/
//                   Multi-Physics
//
//  License:         BSD License
//                     Kratos default license: kratos/license.txt
//
//  Main authors:    Aditya Ghantasala
//
//
#if !defined(KRATOS_RESIDUAL_BASED_BLOCK_BUILDER_AND_SOLVER_WITH_CONSTRAINTS)
#define KRATOS_RESIDUAL_BASED_BLOCK_BUILDER_AND_SOLVER_WITH_CONSTRAINTS

/* System includes */
#include <unordered_set>

/* External includes */
// #define USE_GOOGLE_HASH
#ifdef USE_GOOGLE_HASH
#include "sparsehash/dense_hash_set" //included in external libraries
#endif
// #define USE_LOCKS_IN_ASSEMBLY

/* Project includes */
#include "solving_strategies/builder_and_solvers/residualbased_block_builder_and_solver.h"
#include "includes/master_slave_constraint.h"
#include "utilities/auxilary_global_master_slave_relation.h"

#include "containers/pointer_vector_map.h"
#include "containers/pointer_hash_map_set.h"
#include "containers/data_value_container.h"

namespace Kratos
{

///@name Kratos Globals
///@{

///@}
///@name Type Definitions
///@{

///@}
///@name  Enum's
///@{

///@}
///@name  Functions
///@{

///@}
///@name Kratos Classes
///@{

/**
 * @class ResidualBasedBlockBuilderAndSolverWithConstraints
 * @ingroup KratosCore
 * @brief Current class provides an implementation for standard builder and solving operations.
 * @details The RHS is constituted by the unbalanced loads (residual)
 * Degrees of freedom are reordered putting the restrained degrees of freedom at
 * the end of the system ordered in reverse order with respect to the DofSet.
 * Imposition of the dirichlet conditions is naturally dealt with as the residual already contains
 * this information.
 * Calculation of the reactions involves a cost very similiar to the calculation of the total residual
 * @author Aditya Ghantasala
 */
template <class TSparseSpace,
          class TDenseSpace,  //= DenseSpace<double>,
          class TLinearSolver //= LinearSolver<TSparseSpace,TDenseSpace>
          >
class ResidualBasedBlockBuilderAndSolverWithConstraints
    : public ResidualBasedBlockBuilderAndSolver<TSparseSpace, TDenseSpace, TLinearSolver>
{
  public:
    ///@name Type Definitions
    ///@{
    KRATOS_CLASS_POINTER_DEFINITION(ResidualBasedBlockBuilderAndSolverWithConstraints);

    typedef ResidualBasedBlockBuilderAndSolver<TSparseSpace, TDenseSpace, TLinearSolver> BaseType;

    typedef typename BaseType::TSchemeType TSchemeType;

    typedef typename BaseType::TDataType TDataType;

    typedef typename BaseType::DofsArrayType DofsArrayType;

    typedef typename BaseType::TSystemMatrixType TSystemMatrixType;

    typedef typename BaseType::TSystemVectorType TSystemVectorType;

    typedef typename BaseType::LocalSystemVectorType LocalSystemVectorType;

    typedef typename BaseType::LocalSystemMatrixType LocalSystemMatrixType;

    typedef typename BaseType::TSystemMatrixPointerType TSystemMatrixPointerType;
    typedef typename BaseType::TSystemVectorPointerType TSystemVectorPointerType;

    typedef typename BaseType::NodeType NodeType;

    typedef typename BaseType::NodesArrayType NodesArrayType;
    typedef typename BaseType::ElementsArrayType ElementsArrayType;
    typedef typename BaseType::ConditionsArrayType ConditionsArrayType;

    typedef typename BaseType::ElementsContainerType ElementsContainerType;

    typedef MasterSlaveConstraint MasterSlaveConstraintType;
    typedef typename MasterSlaveConstraint::Pointer MasterSlaveConstraintPointerType;

    typedef AuxilaryGlobalMasterSlaveRelation AuxilaryGlobalMasterSlaveRelationType;

    typedef PointerVectorSet<AuxilaryGlobalMasterSlaveRelationType, IndexedObject> GlobalMasterSlaveRelationContainerType;
    typedef std::size_t SizeType;
    typedef std::vector<SizeType> EquationIdVectorType;
    typedef std::vector<SizeType> VectorIndexType;
    typedef std::vector<Dof<double>::Pointer> DofsVectorType;
    typedef Matrix MatrixType;
    typedef Vector VectorType;
    typedef Geometry<NodeType> GeometryType;

    ///@}
    ///@name Life Cycle
    ///@{

    /** Constructor.
     */
    ResidualBasedBlockBuilderAndSolverWithConstraints(
        typename TLinearSolver::Pointer pNewLinearSystemSolver)
        : ResidualBasedBlockBuilderAndSolver<TSparseSpace, TDenseSpace, TLinearSolver>(pNewLinearSystemSolver)
    {
    }

    /** Destructor.
     */
    ~ResidualBasedBlockBuilderAndSolverWithConstraints() override
    {
    }

    ///@}
    ///@name Operators
    ///@{

    ///@}
    ///@name Operations
    ///@{

    void SetUpSystem(
        ModelPart &rModelPart) override
    {
        BaseType::SetUpSystem(rModelPart);
        if(rModelPart.NumberOfMasterSlaveConstraints() > 0)
        {
            FormulateGlobalMasterSlaveRelations(rModelPart);
        }
    }

    /**
     * @brief Function to perform the build of the RHS. The vector could be sized as the total number
     * of dofs or as the number of unrestrained ones
     * @param pScheme The integration scheme considered
     * @param rModelPart The model part of the problem to solve
     * @param A The LHS matrix
     * @param b The RHS vector
     */
    void Build(
        typename TSchemeType::Pointer pScheme,
        ModelPart &rModelPart,
        TSystemMatrixType &A,
        TSystemVectorType &b) override
    {
        if(rModelPart.NumberOfMasterSlaveConstraints() > 0)
            BuildWithConstraints(pScheme, rModelPart, A, b);
        else
            BaseType::Build(pScheme, rModelPart, A, b);
    }


    /**
     * @brief Function to perform the building and solving phase at the same time.
     * @details It is ideally the fastest and safer function to use when it is possible to solve
     * just after building
     * @param pScheme The integration scheme considered
     * @param rModelPart The model part of the problem to solve
     * @param A The LHS matrix
     * @param Dx The Unknowns vector
     * @param b The RHS vector
     */
    void BuildAndSolve(
        typename TSchemeType::Pointer pScheme,
        ModelPart &rModelPart,
        TSystemMatrixType &A,
        TSystemVectorType &Dx,
        TSystemVectorType &b) override
    {
        if(rModelPart.NumberOfMasterSlaveConstraints() > 0)
            BuildAndSolveWithConstraints(pScheme, rModelPart, A, Dx, b);
        else
            BaseType::BuildAndSolve(pScheme, rModelPart, A, Dx, b);
    }

    void BuildAndSolveWithConstraints(
        typename TSchemeType::Pointer pScheme,
        ModelPart &rModelPart,
        TSystemMatrixType &A,
        TSystemVectorType &Dx,
        TSystemVectorType &b)
    {
        KRATOS_TRY

        Timer::Start("Build");
        this->UpdateConstraintsForBuilding(rModelPart);

        Build(pScheme, rModelPart, A, b);

        Timer::Stop("Build");

        this->ApplyDirichletConditions(pScheme, rModelPart, A, Dx, b);

        KRATOS_INFO_IF("ResidualBasedBlockBuilderAndSolverWithConstraints", (this->GetEchoLevel() == 3)) << "Before the solution of the system"
                                                                                                         << "\nSystem Matrix = " << A << "\nUnknowns vector = " << Dx << "\nRHS vector = " << b << std::endl;

        const double start_solve = OpenMPUtils::GetCurrentTime();
        Timer::Start("Solve");

        this->SystemSolveWithPhysics(A, Dx, b, rModelPart);

        ReconstructSlaveSolutionAfterSolve(rModelPart, A, Dx, b);

        Timer::Stop("Solve");
        const double stop_solve = OpenMPUtils::GetCurrentTime();
        KRATOS_INFO_IF("ResidualBasedBlockBuilderAndSolverWithConstraints", (this->GetEchoLevel() >= 1 && rModelPart.GetCommunicator().MyPID() == 0)) << "System solve time: " << stop_solve - start_solve << std::endl;

        KRATOS_INFO_IF("ResidualBasedBlockBuilderAndSolverWithConstraints", (this->GetEchoLevel() == 3)) << "After the solution of the system"
                                                                                                         << "\nSystem Matrix = " << A << "\nUnknowns vector = " << Dx << "\nRHS vector = " << b << std::endl;

        KRATOS_CATCH("")
    }

    //**************************************************************************
    //**************************************************************************

    void InitializeSolutionStep(
        ModelPart &rModelPart,
        TSystemMatrixType &A,
        TSystemVectorType &Dx,
        TSystemVectorType &b) override
    {
        KRATOS_TRY

        const int n_constraints = static_cast<int>(rModelPart.MasterSlaveConstraints().size());
        ModelPart::MasterSlaveConstraintContainerType::iterator constraints_begin = rModelPart.MasterSlaveConstraintsBegin();
#pragma omp for schedule(guided, 512)
        for (int k = 0; k < n_constraints; k++)
        {
            ModelPart::MasterSlaveConstraintContainerType::iterator it = constraints_begin + k;
            it->InitializeSolutionStep(); // Here each constraint constructs and stores its T and C matrices. Also its equation ids.
        }

        KRATOS_CATCH("")
    }

    //**************************************************************************
    //**************************************************************************

    void FinalizeSolutionStep(
        ModelPart &rModelPart,
        TSystemMatrixType &A,
        TSystemVectorType &Dx,
        TSystemVectorType &b) override
    {
        const int n_constraints = static_cast<int>(rModelPart.MasterSlaveConstraints().size());
        ModelPart::MasterSlaveConstraintContainerType::iterator constraints_begin = rModelPart.MasterSlaveConstraintsBegin();
#pragma omp for schedule(guided, 512)
        for (int k = 0; k < n_constraints; k++)
        {
            ModelPart::MasterSlaveConstraintContainerType::iterator it = constraints_begin + k;
            it->FinalizeSolutionStep();
        }
    }

    ///@}
    ///@name Access
    ///@{

    ///@}
    ///@name Inquiry
    ///@{

    ///@}
    ///@name Friends
    ///@{

    ///@}

  protected:
    ///@name Protected static Member Variables
    ///@{

    ///@}
    ///@name Protected member Variables
    ///@{

    ///@}
    ///@name Protected Operators
    ///@{

    ///@}
    ///@name Protected Operations
    ///@{
    virtual void ConstructMatrixStructure(
        typename TSchemeType::Pointer pScheme,
        TSystemMatrixType &A,
        ModelPart &rModelPart) override
    {
        if(rModelPart.NumberOfMasterSlaveConstraints() > 0)
            ConstructMatrixStructureWithConstraints(pScheme, A, rModelPart);
        else
            BaseType::ConstructMatrixStructure(pScheme, A, rModelPart);
    }

    ///@}
    ///@name Protected  Access
    ///@{

    ///@}
    ///@name Protected Inquiry
    ///@{

    ///@}
    ///@name Protected LifeCycle
    ///@{

    ///@}

  private:
    ///@name Static Member Variables
    ///@{

    ///@}
    ///@name Member Variables
    ///@{

    // This is the set of condenced global constraints.
    GlobalMasterSlaveRelationContainerType mGlobalMasterSlaveRelations; //This can be changed to more efficient implementation later on.

    ///@}
    ///@name Private Operators
    ///@{

    ///@}
    ///@name Private Operations
    ///@{

    /*
    * This function is exactly same as the ConstructMatrixStructure() function in base class except that the function
    * has the call to ApplyConstraints function call once the element and conditions compute their equation ids
    */
    virtual void ConstructMatrixStructureWithConstraints(
        typename TSchemeType::Pointer pScheme,
        TSystemMatrixType &A,
        ModelPart &rModelPart)
    {
        //filling with zero the matrix (creating the structure)
        Timer::Start("MatrixStructure");

        const std::size_t equation_size = BaseType::mEquationSystemSize;

#ifdef USE_GOOGLE_HASH
        std::vector<google::dense_hash_set<std::size_t>> indices(equation_size);
        const std::size_t empty_key = 2 * equation_size + 10;
#else
        std::vector<std::unordered_set<std::size_t>> indices(equation_size);
#endif

#pragma omp parallel for firstprivate(equation_size)
        for (int iii = 0; iii < static_cast<int>(equation_size); iii++)
        {
#ifdef USE_GOOGLE_HASH
            indices[iii].set_empty_key(empty_key);
#else
            indices[iii].reserve(40);
#endif
        }

        Element::EquationIdVectorType ids(3, 0);

        const int nelements = static_cast<int>(rModelPart.Elements().size());
#pragma omp parallel for firstprivate(nelements, ids)
        for (int iii = 0; iii < nelements; iii++)
        {
            typename ElementsContainerType::iterator i_element = rModelPart.Elements().begin() + iii;
            pScheme->EquationId(*(i_element.base()), ids, rModelPart.GetProcessInfo());
            ApplyConstraints<Element>(rModelPart, *i_element, ids, rModelPart.GetProcessInfo());
            for (std::size_t i = 0; i < ids.size(); i++)
            {
#ifdef _OPENMP
                omp_set_lock(&BaseType::mlock_array[ids[i]]);
#endif
                auto &row_indices = indices[ids[i]];
                row_indices.insert(ids.begin(), ids.end());

#ifdef _OPENMP
                omp_unset_lock(&BaseType::mlock_array[ids[i]]);
#endif
            }
        }

        const int nconditions = static_cast<int>(rModelPart.Conditions().size());
#pragma omp parallel for firstprivate(nconditions, ids)
        for (int iii = 0; iii < nconditions; iii++)
        {
            typename ConditionsArrayType::iterator i_condition = rModelPart.Conditions().begin() + iii;
            pScheme->Condition_EquationId(*(i_condition.base()), ids, rModelPart.GetProcessInfo());
            ApplyConstraints<Condition>(rModelPart, *i_condition, ids, rModelPart.GetProcessInfo());
            for (std::size_t i = 0; i < ids.size(); i++)
            {
#ifdef _OPENMP
                omp_set_lock(&BaseType::mlock_array[ids[i]]);
#endif
                auto &row_indices = indices[ids[i]];
                row_indices.insert(ids.begin(), ids.end());
#ifdef _OPENMP
                omp_unset_lock(&BaseType::mlock_array[ids[i]]);
#endif
            }
        }

        //count the row sizes
        unsigned int nnz = 0;
        for (unsigned int i = 0; i < indices.size(); i++)
            nnz += indices[i].size();

        A = boost::numeric::ublas::compressed_matrix<double>(indices.size(), indices.size(), nnz);

        double *Avalues = A.value_data().begin();
        std::size_t *Arow_indices = A.index1_data().begin();
        std::size_t *Acol_indices = A.index2_data().begin();

        //filling the index1 vector - DO NOT MAKE PARALLEL THE FOLLOWING LOOP!
        Arow_indices[0] = 0;
        for (int i = 0; i < static_cast<int>(A.size1()); i++)
            Arow_indices[i + 1] = Arow_indices[i] + indices[i].size();

#pragma omp parallel for
        for (int i = 0; i < static_cast<int>(A.size1()); i++)
        {
            const unsigned int row_begin = Arow_indices[i];
            const unsigned int row_end = Arow_indices[i + 1];
            unsigned int k = row_begin;
            for (auto it = indices[i].begin(); it != indices[i].end(); it++)
            {
                Acol_indices[k] = *it;
                Avalues[k] = 0.0;
                k++;
            }

            indices[i].clear(); //deallocating the memory

            std::sort(&Acol_indices[row_begin], &Acol_indices[row_end]);
        }

        A.set_filled(indices.size() + 1, nnz);

        Timer::Stop("MatrixStructure");
    }

    /*
    * This function is exactly same as the Build() function in base class except that the function
    * has the call to ApplyConstraints function call once the LHS or RHS are computed by elements and conditions
    */
    void BuildWithConstraints(
        typename TSchemeType::Pointer pScheme,
        ModelPart &rModelPart,
        TSystemMatrixType &A,
        TSystemVectorType &b)
    {
        KRATOS_TRY

        KRATOS_ERROR_IF(!pScheme) << "No scheme provided!" << std::endl;

        // Getting the elements from the model
        const int nelements = static_cast<int>(rModelPart.Elements().size());

        // Getting the array of the conditions
        const int nconditions = static_cast<int>(rModelPart.Conditions().size());

        ProcessInfo &CurrentProcessInfo = rModelPart.GetProcessInfo();
        ModelPart::ElementsContainerType::iterator el_begin = rModelPart.ElementsBegin();
        ModelPart::ConditionsContainerType::iterator cond_begin = rModelPart.ConditionsBegin();

        //contributions to the system
        LocalSystemMatrixType LHS_Contribution = LocalSystemMatrixType(0, 0);
        LocalSystemVectorType RHS_Contribution = LocalSystemVectorType(0);

        //vector containing the localization in the system of the different
        //terms
        Element::EquationIdVectorType EquationId;

        // assemble all elements
        double start_build = OpenMPUtils::GetCurrentTime();

#pragma omp parallel firstprivate(nelements, nconditions, LHS_Contribution, RHS_Contribution, EquationId)
        {
#pragma omp for schedule(guided, 512) nowait
            for (int k = 0; k < nelements; k++)
            {
                ModelPart::ElementsContainerType::iterator it = el_begin + k;

                //detect if the element is active or not. If the user did not make any choice the element
                //is active by default
                bool element_is_active = true;
                if ((it)->IsDefined(ACTIVE))
                    element_is_active = (it)->Is(ACTIVE);

                if (element_is_active)
                {
                    //calculate elemental contribution
                    pScheme->CalculateSystemContributions(*(it.base()), LHS_Contribution, RHS_Contribution, EquationId, CurrentProcessInfo);
                    ApplyConstraints<Element>(rModelPart, *it, LHS_Contribution, RHS_Contribution, EquationId, CurrentProcessInfo);

                    //assemble the elemental contribution
#ifdef USE_LOCKS_IN_ASSEMBLY
                    this->Assemble(A, b, LHS_Contribution, RHS_Contribution, EquationId, BaseType::mlock_array);
#else
                    this->Assemble(A, b, LHS_Contribution, RHS_Contribution, EquationId);
#endif
                    // clean local elemental memory
                    pScheme->CleanMemory(*(it.base()));
                }
            }

//#pragma omp parallel for firstprivate(nconditions, LHS_Contribution, RHS_Contribution, EquationId ) schedule(dynamic, 1024)
#pragma omp for schedule(guided, 512)
            for (int k = 0; k < nconditions; k++)
            {
                ModelPart::ConditionsContainerType::iterator it = cond_begin + k;

                //detect if the element is active or not. If the user did not make any choice the element
                //is active by default
                bool condition_is_active = true;
                if ((it)->IsDefined(ACTIVE))
                    condition_is_active = (it)->Is(ACTIVE);

                if (condition_is_active)
                {
                    //calculate elemental contribution
                    pScheme->Condition_CalculateSystemContributions(*(it.base()), LHS_Contribution, RHS_Contribution, EquationId, CurrentProcessInfo);
                    ApplyConstraints<Condition>(rModelPart, *it, LHS_Contribution, RHS_Contribution, EquationId, CurrentProcessInfo);

                    //assemble the elemental contribution
#ifdef USE_LOCKS_IN_ASSEMBLY
                    this->Assemble(A, b, LHS_Contribution, RHS_Contribution, EquationId, BaseType::mlock_array);
#else
                    this->Assemble(A, b, LHS_Contribution, RHS_Contribution, EquationId);
#endif

                    // clean local elemental memory
                    pScheme->CleanMemory(*(it.base()));
                }
            }
        }

        const double stop_build = OpenMPUtils::GetCurrentTime();
        KRATOS_INFO_IF("ResidualBasedBlockBuilderAndSolverWithConstraints", (this->GetEchoLevel() >= 1 && rModelPart.GetCommunicator().MyPID() == 0)) << "Build time: " << stop_build - start_build << std::endl;

        //for (int i = 0; i < A_size; i++)
        //    omp_destroy_lock(&lock_array[i]);
        KRATOS_INFO_IF("ResidualBasedBlockBuilderAndSolverWithConstraints", (this->GetEchoLevel() > 2 && rModelPart.GetCommunicator().MyPID() == 0)) << "Finished parallel building" << std::endl;

        KRATOS_CATCH("")
    }



    /**
     * @brief   this method condences the MasterSlaveConstraints which are added on the rModelPart
     *          into objects of AuxilaryGlobalMasterSlaveRelation. One unique object for each unique slave.
     *          these will be used in the ApplyConstraints functions late on.
     *          matrix and the right hand side
     * @param   rModelPart The model part of the problem to solve
     */
    void FormulateGlobalMasterSlaveRelations(ModelPart &rModelPart)
    {

        // First delete the existing ones
        mGlobalMasterSlaveRelations.clear();
        // Getting the array of the conditions
        const int number_of_constraints = static_cast<int>(rModelPart.MasterSlaveConstraints().size());
        // Getting the beginning iterator

        ModelPart::MasterSlaveConstraintContainerType::iterator constraints_begin = rModelPart.MasterSlaveConstraintsBegin();
        ProcessInfo &r_current_process_info = rModelPart.GetProcessInfo();
        //contributions to the system
        LocalSystemMatrixType relation_matrix = LocalSystemMatrixType(0, 0);
        LocalSystemVectorType constant_vector = LocalSystemVectorType(0);
        EquationIdVectorType slave_equation_ids = EquationIdVectorType(0);
        EquationIdVectorType master_equation_ids = EquationIdVectorType(0);

        for (int i_constraints = 0; i_constraints < number_of_constraints; i_constraints++)
        {
            ModelPart::MasterSlaveConstraintContainerType::iterator it = constraints_begin + i_constraints;

            //detect if the element is active or not. If the user did not make any choice the element
            //is active by default
            bool constraint_is_active = true;
            if ((it)->IsDefined(ACTIVE))
                constraint_is_active = (it)->Is(ACTIVE);

            if (constraint_is_active)
            {
                //get the equation Ids of the constraint
                (*it).EquationIdVector(slave_equation_ids, master_equation_ids, r_current_process_info);
                //calculate constraint's T and b matrices
                (*it).CalculateLocalSystem(relation_matrix, constant_vector, r_current_process_info);

                //assemble the Constraint contribution
                AssembleSlaves(slave_equation_ids, master_equation_ids, relation_matrix);
            }
        }
    }


    /**
     * @brief   this method assembles the given list of slave equation ids and corresponding master equatio ids
     * @param   rSlaveEquationIdVector list of slave equation ids to be assembled.
     * @param   rMasterEquationIdVector list of corresponding master equation ids.
     * @param   rRelationMatrix the matrix relating the rSlaveEquationIdVector with rMasterEquationIdVector
     */
    void AssembleSlaves(EquationIdVectorType& rSlaveEquationIdVector, EquationIdVectorType& rMasterEquationIdVector, LocalSystemMatrixType& rRelationMatrix)
    {
        int slave_count = 0;
        for (auto &slave_equation_id : rSlaveEquationIdVector)
        {
            int master_count = 0;
            auto global_constraint = mGlobalMasterSlaveRelations.find(slave_equation_id);
            if (global_constraint == mGlobalMasterSlaveRelations.end())
            {
                mGlobalMasterSlaveRelations.insert(mGlobalMasterSlaveRelations.begin(), Kratos::make_shared<AuxilaryGlobalMasterSlaveRelationType>(slave_equation_id));
                global_constraint = mGlobalMasterSlaveRelations.find(slave_equation_id);
            }
            for (auto &master_equation_id : rMasterEquationIdVector)
            {
                global_constraint->AddMaster(master_equation_id, rRelationMatrix(slave_count, master_count));
                master_count++;
            }
            slave_count++;
        }
    }


    /**
     * @brief   this method resets the LHS and RHS values of the AuxilaryGlobalMasterSlaveRelation objects
     * @param   rModelPart The model part of the problem to solve
     */
    void ResetConstraintRelations(ModelPart &rModelPart)
    {
        const int number_of_constraints = static_cast<int>(mGlobalMasterSlaveRelations.size());
        // Getting the beginning iterator

        GlobalMasterSlaveRelationContainerType::iterator constraints_begin = mGlobalMasterSlaveRelations.begin();

        for (int i_constraints = 0; i_constraints < number_of_constraints; i_constraints++)
        {
            GlobalMasterSlaveRelationContainerType::iterator it = constraints_begin + i_constraints;
            (*it).SetLHSValue(0.0);
            (*it).SetRHSValue(0.0);
        }
    }

    /**
     * @brief   this method uses the MasterSlaveConstraints objects in rModelPart to reconstruct the LHS and RHS values
     *          of the AuxilaryGlobalMasterSlaveRelation objects. That is the value of Slave as LHS and the T*M+C as RHS value
     * @param   rModelPart The model part of the problem to solve
     */
    void UpdateConstraintsForBuilding(ModelPart &rModelPart)
    {
        // Reset the constraint equations
        ResetConstraintRelations(rModelPart);
        // Getting the array of the conditions
        const int number_of_constraints = static_cast<int>(rModelPart.MasterSlaveConstraints().size());
        // Getting the beginning iterator
        ModelPart::MasterSlaveConstraintContainerType::iterator constraints_begin = rModelPart.MasterSlaveConstraintsBegin();
        ProcessInfo &r_current_process_info = rModelPart.GetProcessInfo();

        for (int i_constraints = 0; i_constraints < number_of_constraints; i_constraints++)
        {
            ModelPart::MasterSlaveConstraintContainerType::iterator it = constraints_begin + i_constraints;

            //detect if the element is active or not. If the user did not make any choice the element
            //is active by default
            bool constraint_is_active = true;
            if ((it)->IsDefined(ACTIVE))
                constraint_is_active = (it)->Is(ACTIVE);

            if (constraint_is_active)
            {
                UpdateMasterSlaveConstraint(*it, r_current_process_info);
            }
        }
    }


    /**
     * @brief   this method uses the MasterSlaveConstraints objects in rModelPart to reconstruct the LHS and RHS values
     *          of the individual AuxilaryGlobalMasterSlaveRelation object. That is the value of Slave as LHS and the T*M+C as RHS value
     * @param   rMasterSlaveConstraint The MasterSlaveConstraint which is to be updated
     */
    void UpdateMasterSlaveConstraint(ModelPart::MasterSlaveConstraintType& rMasterSlaveConstraint, ProcessInfo& rCurrentProcessInfo)
    {
        //contributions to the system
        LocalSystemMatrixType relation_matrix = LocalSystemMatrixType(0, 0);
        LocalSystemVectorType constant_vector = LocalSystemVectorType(0);
        EquationIdVectorType slave_equation_ids = EquationIdVectorType(0);
        EquationIdVectorType master_equation_ids = EquationIdVectorType(0);

        //get the equation Ids of the constraint
        (*it).EquationIdVector(slave_equation_ids, master_equation_ids, rCurrentProcessInfo);
        //calculate constraint's T and b matrices
        (*it).CalculateLocalSystem(relation_matrix, constant_vector, rCurrentProcessInfo);
        // For calculating the constant
        MasterSlaveConstraintType::DofPointerVectorType slave_dofs_vector;
        MasterSlaveConstraintType::DofPointerVectorType master_dofs_vector;
        rMasterSlaveConstraint.GetDofList(slave_dofs_vector, master_dofs_vector, rCurrentProcessInfo);
        double slave_value = 0.0;

        int slave_index = 0;
        for (auto &slave_dof : slave_dofs_vector)
        {
            double slave_value_calc = 0.0;
            for (IndexType master_index = 0; master_index < master_dofs_vector.size(); master_index++)
            {
                slave_value_calc += slave_dof->GetSolutionStepValue() * relation_matrix(slave_index, master_index);
            }
            slave_value_calc += constant_vector[slave_index];
            slave_value = slave_dof->GetSolutionStepValue();
            auto global_constraint = mGlobalMasterSlaveRelations.find(slave_dof->EquationId());
            global_constraint->SetLHSValue(slave_value);
            global_constraint->UpdateRHSValue(slave_value_calc);
            slave_index++;
        }
    }


    /**
     * @brief   This adds the equation IDs of masters of all the slaves correspoining to pCurrentElement to EquationIds
     * @details Here cannot use the pure Geometry because, we would need the dof list from the element/geometry.
     * @param   rModelPart The model part of the problem to solve
     * @param   rCurrentContainer the element or condition where the rEquationIds to be modified for master-slave constraints
     * @param   rEquationIds the equation id vector for the above element or condition
     * @param   rCurrentProcessInfo the current process info
     */
    template <typename TContainerType>
    void ApplyConstraints(ModelPart &rModelPart,
                          TContainerType& rCurrentContainer,
                          typename TContainerType::EquationIdVectorType &rEquationIds,
                          ProcessInfo &rCurrentProcessInfo)
    {
        // If no slave is found for this container , no need of going on
        if (! HasSlaveNode(rCurrentContainer.GetGeometry()))
        {
            return;
        }
        DofsVectorType container_dofs;
        typename TContainerType::EquationIdVectorType master_equation_ids;
        rCurrentContainer.GetDofList(container_dofs, rCurrentProcessInfo);
        IndexType slave_equation_id;

        // For each node check if it is a slave or not If it is .. we change the Transformation matrix
        for (IndexType j = 0; j < container_dofs.size(); j++)
        {
            slave_equation_id = container_dofs[j]->EquationId(); // consider everything as a slave.
            // Get the global constraint equation for this slave.
            auto global_master_slave_constraint = mGlobalMasterSlaveRelations.find(slave_equation_id);
            if (global_master_slave_constraint != mGlobalMasterSlaveRelations.end())
            { // if a equation exists for this slave
                global_master_slave_constraint->EquationIdVector(slave_equation_id, master_equation_ids, rCurrentProcessInfo); // get the slave and master equation ids for this slave.
                for (auto &master_eq_id : master_equation_ids)
                {
                    // Add the current slaves master eq ids to the equation ids
                    rEquationIds.push_back(master_eq_id);
                }
            }
        }
    }

    /**
     * @brief   This function modifies the LHS and RHS of the rCurrentContainer to account for any master-slave constraints its nodes/dofs 
     *          are carrying.
     * @details Here cannot use the pure Geometry because, we would need the dof list from the element/geometry.
     * @param   rModelPart The model part of the problem to solve
     * @param   rCurrentContainer the element or condition where the rEquationIds to be modified for master-slave constraints
     * @param   rLHSContribution the LHS contibution of the rCurrentContainer
     * @param   rRHSContribution the RHS contibution of the rCurrentContainer
     * @param   rEquationIds the equation id vector for the above element or condition
     * @param   rCurrentProcessInfo the current process info
     */
    template <typename TContainerType>
    void ApplyConstraints(ModelPart &rModelPart,
                          TContainerType& rCurrentContainer,
                          LocalSystemMatrixType &rLHSContribution,
                          LocalSystemVectorType &rRHSContribution,
                          typename TContainerType::EquationIdVectorType &rEquationIds,
                          ProcessInfo &rCurrentProcessInfo)
    {
        // This function modifies LHS and RHS contributions with T and C matrices of the constraints present in this current pElement

        KRATOS_TRY
        // If no slave is found for this container , no need of going on
        if (! HasSlaveNode(rCurrentContainer.GetGeometry()))
        {
            return;
        }

        MatrixType lhs_contribution = rLHSContribution;
        VectorType rhs_contribution = rRHSContribution;
        typename TContainerType::EquationIdVectorType equation_ids= rEquationIds;
        MatrixType transformation_matrix_local;
        VectorType constant_vector_local;

        // Saving th original system size
        const SizeType initial_sys_size = rLHSContribution.size1();

        // Formulating which are the internal, slave indices locally.
        VectorIndexType::iterator it;
        VectorIndexType local_index_vector;
        VectorIndexType local_internal_index_vector;
        VectorIndexType local_master_index_vector;
        VectorIndexType local_slave_index_vector;

        int index = 0;
        for (auto &eq_id : rEquationIds)
        {
            auto global_master_slave_constraint = mGlobalMasterSlaveRelations.find(eq_id);
            if (global_master_slave_constraint != mGlobalMasterSlaveRelations.end())
            {
                local_slave_index_vector.push_back(index);
            }
            local_index_vector.push_back(index);
            index++;
        }
        std::sort(local_index_vector.begin(), local_index_vector.end());
        std::sort(local_slave_index_vector.begin(), local_slave_index_vector.end());
        std::set_difference(local_index_vector.begin(), local_index_vector.end(), local_slave_index_vector.begin(), local_slave_index_vector.end(), std::back_inserter(local_internal_index_vector));

        // first fill in the rEquationIds using the above function (overloaded one)
        ApplyConstraints<TContainerType>(rModelPart, rCurrentContainer, rEquationIds, rCurrentProcessInfo); // now rEquationIds has all the slave equation ids appended to it.

        // Get number of master indices for this current container
        const SizeType total_number_of_masters = rEquationIds.size() - initial_sys_size;
        for (unsigned int i = initial_sys_size; i < rEquationIds.size(); i++)
        {
            local_master_index_vector.push_back(i);
        }

        // We resize the LHS and RHS contribution with the master sizes
        const SizeType lhs_size_1 = initial_sys_size + total_number_of_masters;
        const SizeType lhs_size_2 = initial_sys_size + total_number_of_masters;
        transformation_matrix_local.resize(lhs_size_1, lhs_size_2, false); //true for Preserving the data and resizing the matrix
        constant_vector_local.resize(lhs_size_1, false);
        // Making the transformation matrix zero
        for (IndexType m = 0; m < lhs_size_1; m++)
        {
            for (IndexType n = 0; n < lhs_size_2; n++)
            {
                transformation_matrix_local(m, n) = 0.0;
            }
            constant_vector_local(m) = 0.0;
        }

        lhs_contribution.resize(lhs_size_1, lhs_size_2, true); //true for Preserving the data and resizing the matrix
        rhs_contribution.resize(lhs_size_1, true);
        // Making the extra part of matrix zero
        for (IndexType m = initial_sys_size; m < lhs_size_1; m++)
        {
            for (IndexType n = 0; n < lhs_size_2; n++)
            {
                lhs_contribution(m, n) = 0.0;
                lhs_contribution(n, m) = 0.0;
            }
            rhs_contribution(m) = 0.0;
        }

        IndexType slave_equation_id;
        typename TContainerType::EquationIdVectorType master_equation_ids;
        VectorType master_weights_vector;
        double slave_constant;
        int i_masters_total = initial_sys_size;

        for (auto &slave_index : local_slave_index_vector)
        {
            auto global_master_slave_constraint = mGlobalMasterSlaveRelations.find(equation_ids[slave_index]);
            if (global_master_slave_constraint != mGlobalMasterSlaveRelations.end())
            {
                global_master_slave_constraint->EquationIdVector(slave_equation_id, master_equation_ids, rCurrentProcessInfo);
                global_master_slave_constraint->CalculateLocalSystem(master_weights_vector, slave_constant, rCurrentProcessInfo);
                for (IndexType i_master = 0; i_master < master_equation_ids.size(); i_master++)
                {
                    transformation_matrix_local(slave_index, i_masters_total) += master_weights_vector(i_master);
                    i_masters_total++;
                }
                constant_vector_local(slave_index) = slave_constant;
            }
            else
            {
                KRATOS_ERROR << "No master slave constraint equation found for atleast one of the dofs .. !" << std::endl;
            }
        }

        for (auto &master_index : local_master_index_vector)
        {
            transformation_matrix_local(master_index, master_index) = 1.0;
        }

        for (auto &internal_index : local_internal_index_vector)
        {
            transformation_matrix_local(internal_index, internal_index) = 1.0;
        }

        // Here order is important as lhs_contribution should be unodified for usin in calculation
        // of RHS constribution. Later on lhs_contribution is modified to apply the constraint.
        // rhs_h =  T'*(rhs - K*g)
        VectorType temp_vec = ( rhs_contribution - prod(lhs_contribution, constant_vector_local) );
        rhs_contribution = prod( trans(transformation_matrix_local), temp_vec );
        // lhs_h = T'*K*T
        MatrixType temp_mat = prod(lhs_contribution, transformation_matrix_local);
        lhs_contribution = prod( trans(transformation_matrix_local),  temp_mat);

        for (auto &slave_index : local_slave_index_vector)
        {
            for (auto &slave_index_other : local_slave_index_vector)
                lhs_contribution(slave_index, slave_index_other) = rLHSContribution(slave_index, slave_index_other);
        }

        for (auto &slave_index : local_slave_index_vector)
        {
            for (auto &master_index : local_master_index_vector)
            {
                lhs_contribution(slave_index, master_index) = 0.0;
                lhs_contribution(master_index, slave_index) = 0.0;
            }
        }
        for (auto &slave_index : local_slave_index_vector)
        {
            for (auto &internal_index : local_internal_index_vector)
            {
                lhs_contribution(slave_index, internal_index) = 0.0;
                lhs_contribution(internal_index, slave_index) = 0.0;
            }
        }
        for (auto &slave_index : local_slave_index_vector)
        {
            rhs_contribution(slave_index) = 0.0;
        }

        rLHSContribution = lhs_contribution;
        rRHSContribution = rhs_contribution;

        KRATOS_CATCH("Applying Multipoint constraints failed ..");
    }

    /**
     * @brief This method reconstructs the slave solution after Solving. 
     * @param rModelPart Reference to the ModelPart containing the problem.
     * @param A System matrix
     * @param Dx Vector of results (variations on nodal variables)
     * @param b RHS vector (residual)
     */
    void ReconstructSlaveSolutionAfterSolve(
        ModelPart &rModelPart,
        TSystemMatrixType &rA,
        TSystemVectorType &rDx,
        TSystemVectorType &rb)
    {

        const int number_of_constraints = static_cast<int>(mGlobalMasterSlaveRelations.size());
        // Getting the beginning iterator

        GlobalMasterSlaveRelationContainerType::iterator constraints_begin = mGlobalMasterSlaveRelations.begin();
        ProcessInfo &r_current_process_info = rModelPart.GetProcessInfo();
        //contributions to the system
        VectorType master_weights_vector;
        double constant = 0.0;
        double slave_dx_value = 0.0;
        IndexType slave_equation_id = 0;
        EquationIdVectorType master_equation_ids = EquationIdVectorType(0);

        for (int i_constraints = 0; i_constraints < number_of_constraints; i_constraints++)
        {
            GlobalMasterSlaveRelationContainerType::iterator it = constraints_begin + i_constraints;

            slave_dx_value = 0.0;
            //get the equation Ids of the constraint
            (*it).EquationIdVector(slave_equation_id, master_equation_ids, r_current_process_info);
            //calculate constraint's T and b matrices
            (*it).CalculateLocalSystem(master_weights_vector, constant, r_current_process_info);
            int master_index = 0;
            for (auto &master_equation_id : master_equation_ids)
            {
                slave_dx_value += TSparseSpace::GetValue(rDx, master_equation_id) * master_weights_vector(master_index);
                master_index++;
            }
            slave_dx_value += constant;

            rDx[slave_equation_id] = slave_dx_value;
        }
    }

    /**
     * @brief this method checks if any of the nodes of the given rGeometry is marked SLAVE.
     * @param rGeometry The geometry to check for.
     */
    bool HasSlaveNode(GeometryType& rGeometry)
    {
        bool slave_found = false;
        const SizeType number_of_nodes = rGeometry.PointsNumber();
        for (IndexType j = 0; j < number_of_nodes; j++)
        {
            if (rGeometry[j].IsDefined(SLAVE))
                slave_found = rGeometry[j].Is(SLAVE);
            if (slave_found)
            {
                break;
            }
        }
        return slave_found;
    }

    ///@}
    ///@name Private  Access
    ///@{

    ///@}
    ///@name Private Inquiry
    ///@{

    ///@}
    ///@name Un accessible methods
    ///@{

    ///@}

}; /* Class ResidualBasedBlockBuilderAndSolverWithConstraints */

///@}

///@name Type Definitions
///@{

///@}

} /* namespace Kratos.*/

#endif /* KRATOS_RESIDUAL_BASED_BLOCK_BUILDER_AND_SOLVER  defined */
