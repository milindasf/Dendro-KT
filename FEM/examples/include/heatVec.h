//
// Created by milinda on 11/22/18.
//

#ifndef DENDRO_5_0_HEATVEC_H
#define DENDRO_5_0_HEATVEC_H

#include "oda.h"
#include "feVector.h"

namespace HeatEq
{
    template <unsigned int dim>
    class HeatVec : public feVector<HeatVec<dim>,dim> {

    private:

        double * imV1;
        double * imV2;

        ot::DA<dim> * &m_uiOctDA = feVec<dim>::m_uiOctDA;
        Point<dim> &m_uiPtMin = feVec<dim>::m_uiPtMin;
        Point<dim> &m_uiPtMax = feVec<dim>::m_uiPtMax;

        static constexpr unsigned int m_uiDim = dim;

    public:
        HeatVec(ot::DA<dim>* da,unsigned int dof=1);
        ~HeatVec();

        /**@biref elemental compute vec for rhs*/
        virtual void elementalComputeVec(const VECType* in,VECType* out, double*coords=NULL,double scale=1.0) override;


        bool preComputeVec(const VECType* in,VECType* out, double scale=1.0);

        bool postComputeVec(const VECType* in,VECType* out, double scale=1.0);

        /**@brief octree grid x to domin x*/
        double gridX_to_X(double x);
        /**@brief octree grid y to domin y*/
        double gridY_to_Y(double y);
        /**@brief octree grid z to domin z*/
        double gridZ_to_Z(double z);



    };

    template class HeatVec<2u>;
    template class HeatVec<3u>;
    template class HeatVec<4u>;
}



#endif //DENDRO_5_0_HEATVEC_H
