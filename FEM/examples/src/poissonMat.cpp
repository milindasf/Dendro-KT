//
// Created by milinda on 11/21/18.
//

#include "poissonMat.h"
#include "mathUtils.h"

namespace PoissonEq
{

template <unsigned int dim>
PoissonMat<dim>::PoissonMat(ot::DA<dim>* da,unsigned int dof) : feMatrix<PoissonMat<dim>,dim>(da,dof)
{
    const unsigned int nPe=m_uiOctDA->getNumNodesPerElement();
    for (unsigned int d = 0; d < dim-1; d++)
      imV[d] = new double[nPe];

    for (unsigned int d = 0; d < dim; d++)
      Qx[d] = new double[nPe];

}

template <unsigned int dim>
PoissonMat<dim>::~PoissonMat()
{
    for (unsigned int d = 0; d < dim-1; d++)
    {
      delete [] imV[d];
      imV[d] = nullptr;
    }

    for (unsigned int d = 0; d < dim; d++)
    {
      delete [] Qx[d];
      Qx[d] = nullptr;
    }
}

template <unsigned int dim>
void PoissonMat<dim>::elementalMatVec(const VECType* in,VECType* out, double*coords,double scale)
{
    // 1D operators.

    const RefElement* refEl=m_uiOctDA->getReferenceElement();

    const double * Q1d=refEl->getQ1d();
    const double * QT1d=refEl->getQT1d();
    const double * Dg=refEl->getDg1d();
    const double * DgT=refEl->getDgT1d();
    const double * W1d=refEl->getWgq();

    const double * mat1dPtrs[dim];


    const unsigned int eleOrder=refEl->getOrder();
    const unsigned int nPe=intPow(eleOrder+1, dim);
    const unsigned int nrp=eleOrder+1;

    Point<dim> eleMin(&coords[0*m_uiDim]);
    Point<dim> eleMax(&coords[(nPe-1)*m_uiDim]);

    const double refElSz=refEl->getElementSz();

    // Take derivatives.
    for (unsigned int d = 0; d < dim; d++)
      mat1dPtrs[d] = Q1d;
    for (unsigned int d = 0; d < dim; d++)
    {
      const double * imFromPtrs[dim];
      double * imToPtrs[dim];

      mat1dPtrs[d] = Dg;
      getImPtrs(imFromPtrs, imToPtrs, in, Qx[d]);
      KroneckerProduct<dim, double, true>(nrp, mat1dPtrs, imFromPtrs, imToPtrs);
      mat1dPtrs[d] = Q1d;
    }

    //Backup
    /// //x derivative
    /// DENDRO_TENSOR_IIAX_APPLY_ELEM(nrp,Dg,in,imV1);
    /// DENDRO_TENSOR_IAIX_APPLY_ELEM(nrp,Q1d,imV1,imV2);
    /// DENDRO_TENSOR_AIIX_APPLY_ELEM(nrp,Q1d,imV2,Qx);

    /// //y derivative
    /// DENDRO_TENSOR_IIAX_APPLY_ELEM(nrp,Q1d,in,imV1);
    /// DENDRO_TENSOR_IAIX_APPLY_ELEM(nrp,Dg,imV1,imV2);
    /// DENDRO_TENSOR_AIIX_APPLY_ELEM(nrp,Q1d,imV2,Qy);

    /// //z derivative
    /// DENDRO_TENSOR_IIAX_APPLY_ELEM(nrp,Q1d,in,imV1);
    /// DENDRO_TENSOR_IAIX_APPLY_ELEM(nrp,Q1d,imV1,imV2);
    /// DENDRO_TENSOR_AIIX_APPLY_ELEM(nrp,Dg,imV2,Qz);


    const Point<dim> sz = gridX_to_X(eleMax) - gridX_to_X(eleMin);
    const Point<dim> J = sz * (1.0 / refElSz);

    //TODO what was this representing mathematically? Need to generalize correctly.
    double J_quotient[dim];
    for (unsigned int d = 0; d < dim; d++)
    {
      J_quotient[d] = 1.0;
      for (unsigned int dd = 0; dd < dim; dd++)
        if (dd != d)
          J_quotient[d] *= J.x(dd);
        else
          J_quotient[d] /= J.x(dd);
    }

    //std::cout<<"Stifness:  elem: "<<elem<<" ele Sz: "<<(elem.maxX()-elem.minX())<<" szX: "<<szX<<" Jx: "<<Jx<<" J: "<<(Jx*Jy*Jz)<<std::endl;

    for (unsigned int d = 0; d < dim; d++)
      SymmetricOuterProduct<double, dim>::applyHadamardProduct(eleOrder+1, Qx[d], W1d, J_quotient[d]);

    // Backup.
    /// for(unsigned int k=0;k<(eleOrder+1);k++)
    ///     for(unsigned int j=0;j<(eleOrder+1);j++)
    ///         for(unsigned int i=0;i<(eleOrder+1);i++)
    ///         {
    ///             Qx[k*(eleOrder+1)*(eleOrder+1)+j*(eleOrder+1)+i]*=( ((Jy*Jz)/Jx)*W1d[i]*W1d[j]*W1d[k]);
    ///             Qy[k*(eleOrder+1)*(eleOrder+1)+j*(eleOrder+1)+i]*=( ((Jx*Jz)/Jy)*W1d[i]*W1d[j]*W1d[k]);
    ///             Qz[k*(eleOrder+1)*(eleOrder+1)+j*(eleOrder+1)+i]*=( ((Jx*Jy)/Jz)*W1d[i]*W1d[j]*W1d[k]);
    ///         }


    for (unsigned int d = 0; d < dim; d++)
      mat1dPtrs[d] = QT1d;
    for (unsigned int d = 0; d < dim; d++)
    {
      const double * imFromPtrs[dim];
      double * imToPtrs[dim];

      mat1dPtrs[d] = DgT;
      getImPtrs(imFromPtrs, imToPtrs, Qx[d], Qx[d]);
      KroneckerProduct<dim, double, true>(nrp, mat1dPtrs, imFromPtrs, imToPtrs);
      mat1dPtrs[d] = QT1d;
    }

    // Backup.
    /// DENDRO_TENSOR_IIAX_APPLY_ELEM(nrp,DgT,Qx,imV1);
    /// DENDRO_TENSOR_IAIX_APPLY_ELEM(nrp,QT1d,imV1,imV2);
    /// DENDRO_TENSOR_AIIX_APPLY_ELEM(nrp,QT1d,imV2,Qx);

    /// DENDRO_TENSOR_IIAX_APPLY_ELEM(nrp,QT1d,Qy,imV1);
    /// DENDRO_TENSOR_IAIX_APPLY_ELEM(nrp,DgT,imV1,imV2);
    /// DENDRO_TENSOR_AIIX_APPLY_ELEM(nrp,QT1d,imV2,Qy);

    /// DENDRO_TENSOR_IIAX_APPLY_ELEM(nrp,QT1d,Qz,imV1);
    /// DENDRO_TENSOR_IAIX_APPLY_ELEM(nrp,QT1d,imV1,imV2);
    /// DENDRO_TENSOR_AIIX_APPLY_ELEM(nrp,DgT,imV2,Qz);

    for(unsigned int i=0;i<nPe;i++)
    {
      out[i] = Qx[0][i];
      for (unsigned int d = 1; d < dim; d++)
        out[i]=Qx[d][i];
    }
}


template <unsigned int dim>
bool PoissonMat<dim>::preMatVec(const VECType* in,VECType* out,double scale)
{
    // apply boundary conditions.
    std::vector<unsigned int> bdyIndex;
    m_uiOctDA->getBoundaryNodeIndices(bdyIndex);

    for(unsigned int i=0;i<bdyIndex.size();i++)
        out[bdyIndex[i]]=0.0;
}

template <unsigned int dim>
bool PoissonMat<dim>::postMatVec(const VECType* in,VECType* out,double scale) {

    // apply boundary conditions.
    std::vector<unsigned int> bdyIndex;
    m_uiOctDA->getBoundaryNodeIndices(bdyIndex);

    for(unsigned int i=0;i<bdyIndex.size();i++)
        out[bdyIndex[i]]=0.0;
}

template <unsigned int dim>
double PoissonMat<dim>::gridX_to_X(unsigned int d, double x) const
{
  double Rg=1.0;
  return (((x)/(Rg))*((m_uiPtMax.x(d)-m_uiPtMin.x(d)))+m_uiPtMin.x(d));
}

template <unsigned int dim>
Point<dim> PoissonMat<dim>::gridX_to_X(Point<dim> x) const
{
  double newCoords[dim];
  for (unsigned int d = 0; d < dim; d++)
    newCoords[d] = gridX_to_X(d, x.x(d));
  return Point<dim>(newCoords);
}

template <unsigned int dim>
int PoissonMat<dim>::cgSolve(double * x ,double * b,int max_iter, double& tol,unsigned int var)
{
    double resid,alpha,beta,rho,rho_1;
    int status=1; // 0 indicates it has solved the system within the specified max_iter, 1 otherwise.

    const unsigned int local_dof=m_uiOctDA->getLocalNodalSz();

    MPI_Comm globalComm=m_uiOctDA->getGlobalComm();

    if(m_uiOctDA->isActive())
    {

        int activeRank=m_uiOctDA->getRankActive();
        int activeNpes=m_uiOctDA->getNpesActive();

        MPI_Comm activeComm=m_uiOctDA->getCommActive();

        double* p;
        double* z;
        double* q;
        double* Ax;
        double* Ap;
        double* r0;
        double* r1;

        m_uiOctDA->createVector(p);
        m_uiOctDA->createVector(z);
        m_uiOctDA->createVector(q);

        m_uiOctDA->createVector(Ax);
        m_uiOctDA->createVector(Ap);
        m_uiOctDA->createVector(r0);
        m_uiOctDA->createVector(r1);

        double normb = normLInfty(b,local_dof,activeComm);
        par::Mpi_Bcast(&normb,1,0,activeComm);

        if(!activeRank)
            std::cout<<"normb = "<<normb<<std::endl;

        this->matVec(x,Ax);

        /*char fPrefix[256];
        sprintf(fPrefix,"%s_%d","cg",0);
        const char * varNames[]={"U"};
        const double * var[]={Ax};
        io::vtk::mesh2vtuFine(mesh,fPrefix,0,NULL,NULL,1,varNames,var);
*/
        for(unsigned int i=0;i<local_dof;i++)
        {
            r0[i]=b[i]-Ax[i];
            p[i]=r0[i];
        }


        if (normb == 0.0)
            normb = 1;

        double normr=normLInfty(r0,local_dof,activeComm);
        par::Mpi_Bcast(&normr,1,0,activeComm);
        if(!activeRank) std::cout<<"initial residual : "<<(normr/normb)<<std::endl;

        if ((resid = normr / normb) <= tol) {
            tol = resid;
            max_iter = 0;

            m_uiOctDA->destroyVector(p);
            m_uiOctDA->destroyVector(z);
            m_uiOctDA->destroyVector(q);

            m_uiOctDA->destroyVector(Ax);
            m_uiOctDA->destroyVector(Ap);
            m_uiOctDA->destroyVector(r0);
            m_uiOctDA->destroyVector(r1);

            status=0;
        }

        if(status!=0)
        {

            for(unsigned int i=1;i<=max_iter;i++)
            {

                this->matVec(p,Ap);

                alpha=(dot(r0,r0,local_dof,activeComm)/dot(p,Ap,local_dof,activeComm));
                par::Mpi_Bcast(&alpha,1,0,activeComm);

                //if(!activeRank) std::cout<<"rank: " <<activeRank<<" alpha: "<<alpha<<std::endl;
                for(unsigned int e=0;e<local_dof;e++)
                {
                    x[e]+=alpha*p[e];
                    r1[e]=r0[e]-alpha*Ap[e];
                }

                normr=normLInfty(r1,local_dof,activeComm);
                par::Mpi_Bcast(&normr,1,0,activeComm);

                if((!activeRank) && (i%10==0)) std::cout<<" iteration : "<<i<<" residual : "<<resid<<std::endl;

                if ((resid = normr / normb) <= tol) {

                    if((!activeRank)) std::cout<<" iteration : "<<i<<" residual : "<<resid<<std::endl;
                    tol = resid;
                    m_uiOctDA->destroyVector(p);
                    m_uiOctDA->destroyVector(z);
                    m_uiOctDA->destroyVector(q);

                    m_uiOctDA->destroyVector(Ax);
                    m_uiOctDA->destroyVector(Ap);
                    m_uiOctDA->destroyVector(r0);
                    m_uiOctDA->destroyVector(r1);

                    status=0;
                    break;
                }

                beta=(dot(r1,r1,local_dof,activeComm)/dot(r0,r0,local_dof,activeComm));
                par::Mpi_Bcast(&beta,1,0,activeComm);

                //if(!activeRank) std::cout<<"<r_1,r_1> : "<<dot(r1+nodeLocalBegin,r1+nodeLocalBegin,local_dof,activeComm)<<" <r_0,r_0>: "<<dot(r0+nodeLocalBegin,r0+nodeLocalBegin,local_dof,activeComm)<<" beta "<<beta<<std::endl;



                for(unsigned int e=0;e<local_dof;e++)
                {
                    p[e]=r1[e]+beta*p[e];
                    r0[e]=r1[e];
                }


            }

            if(status!=0)
            {
                tol = resid;
                m_uiOctDA->destroyVector(p);
                m_uiOctDA->destroyVector(z);
                m_uiOctDA->destroyVector(q);

                m_uiOctDA->destroyVector(Ax);
                m_uiOctDA->destroyVector(Ap);
                m_uiOctDA->destroyVector(r0);
                m_uiOctDA->destroyVector(r1);
                status=1;

            }



        }


    }


    // bcast act as a barrier for active and inactive meshes.
    par::Mpi_Bcast(&tol,1,0,globalComm);
    return status;
}

}//namespace PoissonEq