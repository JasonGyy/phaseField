// created: 2016-4-21 13:56:30
// version: master
// url: https://github.com/bpuchala/IntegrationToolsWriter.git
// commit: 8a15adf67355fad30bd75ce9ba6b1f8d24b9a537

#ifndef pfunct_faV_HH
#define pfunct_faV_HH

#include <cmath>
#include <cstdlib>
#include "IntegrationTools/PFunction.hh"

namespace PRISMS
{
    template< class VarContainer>
    class pfunct_faV_f : public PSimpleBase< VarContainer, double>
    {
        double eval( const VarContainer &var) const
        {
            return  -1.6070000000000000e+00*var[0]+4.6598999999999997e+01*(var[0]*var[0])+1.0827460000000000e-04;
        }

    public:

        pfunct_faV_f()
        {
            this->_name = "pfunct_faV_f";
        }

        std::string csrc() const
        {
            return " -1.6070000000000000e+00*var[0]+4.6598999999999997e+01*(var[0]*var[0])+1.0827460000000000e-04";
        }

        std::string sym() const
        {
            return "1.082746E-4-(1.607)*c+(46.599)*c^2";
        }

        std::string latex() const
        {
            return "1.082746E-4+{(46.599)} c^{2}-{(1.607)} c";
        }

        pfunct_faV_f* clone() const
        {
            return new pfunct_faV_f(*this);
        }
    };

    template< class VarContainer>
    class pfunct_faV_grad_0 : public PSimpleBase< VarContainer, double>
    {
        double eval( const VarContainer &var) const
        {
            return  9.3197999999999993e+01*var[0]-1.6070000000000000e+00;
        }

    public:

        pfunct_faV_grad_0()
        {
            this->_name = "pfunct_faV_grad_0";
        }

        std::string csrc() const
        {
            return " 9.3197999999999993e+01*var[0]-1.6070000000000000e+00";
        }

        std::string sym() const
        {
            return "-1.607+(93.198)*c";
        }

        std::string latex() const
        {
            return "-1.607+{(93.198)} c";
        }

        pfunct_faV_grad_0* clone() const
        {
            return new pfunct_faV_grad_0(*this);
        }
    };

    template< class VarContainer>
    class pfunct_faV_hess_0_0 : public PSimpleBase< VarContainer, double>
    {
        double eval( const VarContainer &var) const
        {
            return 9.3197999999999993e+01;
        }

    public:

        pfunct_faV_hess_0_0()
        {
            this->_name = "pfunct_faV_hess_0_0";
        }

        std::string csrc() const
        {
            return "9.3197999999999993e+01";
        }

        std::string sym() const
        {
            return "93.198";
        }

        std::string latex() const
        {
            return "93.198";
        }

        pfunct_faV_hess_0_0* clone() const
        {
            return new pfunct_faV_hess_0_0(*this);
        }
    };

    template<class VarContainer>
    class pfunct_faV : public PFuncBase< VarContainer, double>
    {
    public:
        
        typedef typename PFuncBase< VarContainer, double>::size_type size_type;

        PSimpleBase< VarContainer, double> *_val;
        PSimpleBase< VarContainer, double> **_grad_val;
        PSimpleBase< VarContainer, double> ***_hess_val;
        
        pfunct_faV()
        {
            construct();
        }

        pfunct_faV(const pfunct_faV &RHS )
        {
            construct(false);
            
            _val = RHS._val->clone();
            _grad_val[0] = RHS._grad_val[0]->clone();
            _hess_val[0][0] = RHS._hess_val[0][0]->clone();
            
        }

        pfunct_faV& operator=( pfunct_faV RHS )
        {
            using std::swap;
            
            swap(_val, RHS._val);
            swap(_grad_val[0], RHS._grad_val[0]);
            swap(_hess_val[0][0], RHS._hess_val[0][0]);
            
            return *this;
        }

        ~pfunct_faV()
        {
            delete _val;

            delete _grad_val[0];
            delete [] _grad_val;

            delete _hess_val[0][0];
            delete [] _hess_val[0];
            delete [] _hess_val;
        }

        pfunct_faV<VarContainer>* clone() const
        {
            return new pfunct_faV<VarContainer>(*this);
        }

        PSimpleFunction< VarContainer, double> simplefunction() const
        {
            return PSimpleFunction< VarContainer, double>( *_val );
        }

        PSimpleFunction< VarContainer, double> grad_simplefunction(size_type di) const
        {
            return PSimpleFunction< VarContainer, double>( *_grad_val[di] );
        }

        PSimpleFunction< VarContainer, double> hess_simplefunction(size_type di, size_type dj) const
        {
            return PSimpleFunction< VarContainer, double>( *_hess_val[di][dj] );
        }

        double operator()(const VarContainer &var)
        {
            return (*_val)(var);
        }

        double grad(const VarContainer &var, size_type di)
        {
            return (*_grad_val[di])(var);
        }

        double hess(const VarContainer &var, size_type di, size_type dj)
        {
            return (*_hess_val[di][dj])(var);
        }

        void eval(const VarContainer &var)
        {
            (*_val)(var);
        }

        void eval_grad(const VarContainer &var)
        {
            (*_grad_val[0])(var);
        }

        void eval_hess(const VarContainer &var)
        {
            (*_hess_val[0][0])(var);
        }

        double operator()() const
        {
            return (*_val)();
        }

        double grad(size_type di) const
        {
            return (*_grad_val[di])();
        }

        double hess(size_type di, size_type dj) const
        {
            return (*_hess_val[di][dj])();
        }

    private:
        void construct(bool allocate = true)
        {
            this->_name = "pfunct_faV";
            this->_var_name.clear();
            this->_var_name.push_back("c");
            this->_var_description.clear();
            this->_var_description.push_back("concentration");
            
            _grad_val = new PSimpleBase< VarContainer, double>*[1];
            
            _hess_val = new PSimpleBase< VarContainer, double>**[1];
            _hess_val[0] = new PSimpleBase< VarContainer, double>*[1];
            
            if(!allocate) return;
            
            _val = new pfunct_faV_f<VarContainer>();
            
            _grad_val[0] = new pfunct_faV_grad_0<VarContainer>();
            
            _hess_val[0][0] = new pfunct_faV_hess_0_0<VarContainer>();
        }

    };


}
#endif
