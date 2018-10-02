// KRATOS  ___|  |                   |                   |
//       \___ \  __|  __| |   |  __| __| |   |  __| _` | |
//             | |   |    |   | (    |   |   | |   (   | |
//       _____/ \__|_|   \__,_|\___|\__|\__,_|_|  \__,_|_| MECHANICS
//
//  License:         BSD License
//                   license: structural_mechanics_application/license.txt
//
//  Main authors:    Vicente Mataix Ferrandiz
//                   Alejandro Cornejo
//                   Lucia Barbu
//

#if !defined(KRATOS_FINITE_STRAIN_TRESCA_PLASTIC_POTENTIAL_H_INCLUDED)
#define KRATOS_FINITE_STRAIN_TRESCA_PLASTIC_POTENTIAL_H_INCLUDED

// System includes

// Project includes
#include "custom_constitutive/plastic_potentials/finite_strain/generic_plastic_potential.h"

namespace Kratos
{
///@name Kratos Globals
///@{

///@}
///@name Type Definitions
///@{

    // The size type definition
    typedef std::size_t SizeType;
    
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
 * @class FiniteStrainTrescaPlasticPotential
 * @ingroup StructuralMechanicsApplication
 * @brief This class defines a plastic potential following the theory of Tresca
 * @details Working from the conventional assumption that the strength is related to the difference between major and minor principal stresses results in the Tresca model for total stress. This gives a hexagonal form of the potential in the principal stress space
 * @author Vicente Mataix Ferrandiz
 * @author Alejandro Cornejo
 * @author Lucia Barbu
 */
template <SizeType TVoigtSize = 6>
class KRATOS_API(STRUCTURAL_MECHANICS_APPLICATION) FiniteStrainTrescaPlasticPotential
{
  public:
    ///@name Type Definitions
    ///@{

    /// We define the dimension
    static constexpr SizeType Dimension = TVoigtSize == 6 ? 3 : 2;
      
    /// The define the Voigt size
    static constexpr SizeType VoigtSize = TVoigtSize;
      
    /// The definition of the Voigt array type
    typedef array_1d<double, VoigtSize> BoundedArrayType;

    /// The definition of the bounded matrix type
    typedef BoundedMatrix<double, Dimension, Dimension> BoundedMatrixType;

    /// Counted pointer of FiniteStrainTrescaPlasticPotential
    KRATOS_CLASS_POINTER_DEFINITION(FiniteStrainTrescaPlasticPotential);

    ///@}
    ///@name Life Cycle
    ///@{

    /// Initialization constructor.
    FiniteStrainTrescaPlasticPotential()
    {
    }

    /// Copy constructor
    FiniteStrainTrescaPlasticPotential(FiniteStrainTrescaPlasticPotential const &rOther)
    {
    }

    /// Assignment operator
    FiniteStrainTrescaPlasticPotential &operator=(FiniteStrainTrescaPlasticPotential const &rOther)
    {
        return *this;
    }

    /// Destructor
    virtual ~FiniteStrainTrescaPlasticPotential(){};

    ///@}
    ///@name Operators
    ///@{

    ///@}
    ///@name Operations
    ///@{

    /**
     * @brief This  script  calculates  the derivatives  of the plastic potential
    according   to   NAYAK-ZIENKIEWICZ   paper International
    journal for numerical methods in engineering vol 113-135 1972.
     As:            DF/DS = c1*V1 + c2*V2 + c3*V3
     * @param rPredictiveStressVector The predictive stress vector S = C:(E-Ep)
     * @param rDeviator The deviatoric part of the stress vector
     * @param J2 The second invariant of the Deviator
     * @param rDerivativePlasticPotential The derivative of the plastic potential
     * @param rValues Parameters of the constitutive law
     */
    static void CalculatePlasticPotentialDerivative(
        const BoundedArrayType& rPredictiveStressVector,
        const BoundedArrayType& rDeviator,
        const double J2,
        BoundedMatrixType& rDerivativePlasticPotential,
        ConstitutiveLaw::Parameters& rValues
        )
    {
//         BoundedArrayType first_vector, second_vector, third_vector;
//
//         ConstitutiveLawUtilities<VoigtSize>::CalculateFirstVector(first_vector);
//         ConstitutiveLawUtilities<VoigtSize>::CalculateSecondVector(rDeviator, J2, second_vector);
//         ConstitutiveLawUtilities<VoigtSize>::CalculateThirdVector(rDeviator, J2, third_vector);
//
//         double J3, lode_angle;
//         ConstitutiveLawUtilities<VoigtSize>::CalculateJ3Invariant(rDeviator, J3);
//         ConstitutiveLawUtilities<VoigtSize>::CalculateLodeAngle(J2, J3, lode_angle);
//
//         const double checker = std::abs(lode_angle * 180.0 / Globals::Pi);
//
//         const double c1 = 0.0;
//         double c2, c3;
//
//         if (checker < 29.0) {
//             c2 = 2.0 * (std::cos(lode_angle) + std::sin(lode_angle) * std::tan(3.0 * lode_angle));
//             c3 = std::sqrt(3.0) * std::sin(lode_angle) / (J2 * std::cos(3.0 * lode_angle));
//         } else {
//             c2 = std::sqrt(3.0);
//             c3 = 0.0;
//         }
//
//         noalias(rDerivativePlasticPotential) = c1 * first_vector + c2 * second_vector + c3 * third_vector;
    }

    /**
     * @brief This method defines the check to be performed in the plastic potential
     * @return 0 if OK, 1 otherwise
     */
    static int Check(const Properties& rMaterialProperties)
    {
        return 0;
    }

    ///@}
    ///@name Access
    ///@{

    ///@}
    ///@name Inquiry
    ///@{

    ///@}
    ///@name Input and output
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

    ///@}
    ///@name Private Operators
    ///@{

    ///@}
    ///@name Private Operations
    ///@{

    ///@}
    ///@name Private  Access
    ///@{

    ///@}
    ///@name Private Inquiry
    ///@{

    ///@}
    ///@name Un accessible methods
    ///@{

    // Serialization

    friend class Serializer;

    void save(Serializer &rSerializer) const
    {
    }

    void load(Serializer &rSerializer)
    {
    }

    ///@}

}; // Class FiniteStrainTrescaPlasticPotential

///@}

///@name Type Definitions
///@{

///@}
///@name Input and output
///@{

///@}

} // namespace Kratos.
#endif
