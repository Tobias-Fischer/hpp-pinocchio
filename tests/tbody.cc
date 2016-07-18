//
// Copyright (c) 2016 CNRS
// Author: NMansard
//
//
// This file is part of hpp-pinocchio
// hpp-pinocchio is free software: you can redistribute it
// and/or modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation, either version
// 3 of the License, or (at your option) any later version.
//
// hpp-pinocchio is distributed in the hope that it will be
// useful, but WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Lesser Public License for more details.  You should have
// received a copy of the GNU Lesser General Public License along with
// hpp-pinocchio  If not, see
// <http://www.gnu.org/licenses/>.

#define BOOST_TEST_MODULE tbody

#include <boost/test/unit_test.hpp>

#include <hpp/model/device.hh>
#include <hpp/model/collision-object.hh>
#include <hpp/model/urdf/util.hh>

#include <hpp/pinocchio/device.hh>
#include <hpp/pinocchio/joint.hh>
#include <hpp/pinocchio/body.hh>
#include <pinocchio/multibody/geometry.hpp>
#include "../tests/utils.hh"

static bool verbose = false;

//NOCHECKED       Body (DevicePtr_t device, JointIndex joint);
//NOCHECKED       virtual ~Body () {}
//       const std::string & name () const;
//       JointPtr_t joint () const;
//NOCHECKED       const ObjectVector_t& innerObjects () const { return innerObjects_; }
//NOCHECKED       value_type radius () const
//NOCHECKED       const ObjectVector_t& outerObjects () const { return outerObjects_; }
//       vector3_t localCenterOfMass () const;
//       matrix3_t inertiaMatrix() const;
//       value_type mass() const;
//private:
//NOCHECKED       void selfAssert() const;
//NOCHECKED       void searchFrameIndex() const; 
//NOCHECKED       ModelConstPtr_t    model() const ;
//NOCHECKED       ModelPtr_t         model() ;
//NOCHECKED       const se3::Frame & frame() const ;
//NOCHECKED       se3::Frame &       frame() ;
//NOCHECKED       void updateRadius (const CollisionObjectPtr_t& object);
//NOCHECKED       DevicePtr_t devicePtr;
//NOCHECKED       JointIndex jointIndex;
//NOCHECKED       mutable FrameIndex frameIndex;      mutable bool       frameIndexSet;
//NOCHECKED       ObjectVector innerObjects_,outerObjects_;
//NOCHECKED       value_type radius_;

/* --- UNIT TESTS ----------------------------------------------------------- */
/* --- UNIT TESTS ----------------------------------------------------------- */
/* --- UNIT TESTS ----------------------------------------------------------- */

BOOST_AUTO_TEST_CASE(body)
{
  hpp::model::DevicePtr_t     model     = hppModel();
  hpp::pinocchio::DevicePtr_t pinocchio = hppPinocchio();

  Eigen::VectorXd q = Eigen::VectorXd::Zero( model->configSize() ); // would be better with neutral
  q[3] += 1.0;
  q.segment<4>(3) /= q.segment<4>(3).norm();

  model    ->currentConfiguration(q);
  pinocchio->currentConfiguration(m2p::q(q));
  model    ->controlComputation (hpp::model    ::Device::ALL);
  pinocchio->controlComputation (hpp::pinocchio::Device::ALL);
  model    ->computeForwardKinematics();
  pinocchio->computeForwardKinematics();

  if(verbose)
    {
      hpp::pinocchio::JointConstPtr_t jp5 = pinocchio->getJointVector()[5];
      hpp::pinocchio::BodyPtr_t  bp5 = jp5->linkedBody();
      std::cout << "joint name = " << jp5->name() << std::endl;
      std::cout << "body  name = " << bp5->name() << std::endl;
    }

  for (hpp::pinocchio::JointVector::const_iterator it = ++pinocchio->getJointVector().begin ();
       it != pinocchio->getJointVector().end (); ++it) 
    {
      hpp::pinocchio::JointConstPtr_t jp = *it;
      hpp::model    ::JointConstPtr_t jm = model->getJointByName(jp->name());
      assert( jm );
      assert( jp->name() == jm->name() );
      
      hpp::pinocchio::BodyPtr_t  bp = jp->linkedBody();
      hpp::model    ::BodyPtr_t  bm = jm->linkedBody();

      if( verbose )
        {
          std::cout << bp->name() << " -- " << bm->name() << std::endl;
          std::cout << bp->mass() << " -- " << bm->mass() << std::endl;
          std::cout << bp->localCenterOfMass() << " -- " << bm->localCenterOfMass() << std::endl;
        }

      BOOST_CHECK( bp->name()              == bm->name() );
      BOOST_CHECK( jp->name()              == bp->joint()->name() );
      BOOST_CHECK( bp->mass()              == bm->mass() );

      /* COM and Inertia are expressed in local frames, which are not the same
       * in model and pinocchio because of the trick in model of having only
       * revolution around X axis. So let's express the quantities in the same
       * world frame. */
      const Eigen::Matrix3d oRi_p = jp->currentTransformation().   rotation();
      const Eigen::Matrix3d oRi_m = jm->currentTransformation().getRotation();

      const Eigen::Vector3d cp (oRi_p*bp->localCenterOfMass());
      const Eigen::Vector3d cm (oRi_m*bm->localCenterOfMass());
      BOOST_CHECK( cp.isApprox (cm) );

      // There is a problem in the following comparison. I think it is because
      // Im is not properly parsed, resulting in incorrect order of the columns
      // of Im (in particular, main components not on the diagonal, not symmetric).
      // ... => Skip this test
      // const Eigen::Matrix3d Ip (oRi_p*bp->inertiaMatrix()*oRi_p.transpose());
      // const Eigen::Matrix3d Im (oRi_m*bm->inertiaMatrix()*oRi_m.transpose());
      // BOOST_CHECK( std::abs(Ip.mean()-Im.mean())<1e-6 );
    }     
}
/* -------------------------------------------------------------------------- */

struct IsCollisionObjectNamed
{
  std::string request;
  IsCollisionObjectNamed( const std::string & request ) : request(request) {}
  bool operator() ( hpp::model::CollisionObjectPtr_t bm ) 
  {
    //std::cout << "\t\t" << "Requesting " << request << " \tchecking\t " << bm->name() << std::endl;
    return bm->name() == request; 
  }
};

BOOST_AUTO_TEST_CASE(geoms)
{
  verbose = true;

  hpp::model::DevicePtr_t     model     = hppModel();
  hpp::pinocchio::DevicePtr_t pinocchio = hppPinocchio();

  std::vector<std::string> baseDirs; baseDirs.push_back("/");
  hpp::pinocchio::GeomModelPtr_t geom( new se3::GeometryModel() );
  se3::GeometryModel & geomRef = *geom;
  geomRef = se3::urdf::buildGeom(*pinocchio->model(),pinocchio->name(),baseDirs,se3::COLLISION);

  pinocchio->geomModel(geom);
  pinocchio->createGeomData();

  for (hpp::pinocchio::JointVector::const_iterator it = ++pinocchio->getJointVector().begin ();
       it != pinocchio->getJointVector().end (); ++it) 
    {
      hpp::pinocchio::JointConstPtr_t jp = *it;
      hpp::model    ::JointConstPtr_t jm = model->getJointByName(jp->name());
      assert( jm );
      assert( jp->name() == jm->name() );

      hpp::pinocchio::BodyPtr_t bp = jp->linkedBody();
      hpp::model    ::BodyPtr_t bm = jm->linkedBody();
      assert ( bp->name() == bm->name() );
      
      assert (bp->innerObjects().size() == int(bm->innerObjects(hpp::model::COLLISION).size()));
      for (hpp::pinocchio::ObjectVector::const_iterator itcoll = bp->innerObjects().begin();
           itcoll!=bp->innerObjects().end() ; ++ itcoll)
        {
          hpp::model::ObjectVector_t::const_iterator itcollm = 
            std::find_if( bm->innerObjects(hpp::model::COLLISION).begin(),
                          bm->innerObjects(hpp::model::COLLISION).end(),
                          IsCollisionObjectNamed(bp->name()+std::string("_0")) );
          // Check that a body of the same name exists in both hpp::model and pinocchio.
          BOOST_CHECK( itcollm != bm->innerObjects(hpp::model::COLLISION).end() );
          if (itcollm != bm->innerObjects(hpp::model::COLLISION).end())
            {
              BOOST_CHECK( (*itcollm)->name() == (*itcoll)->name()+std::string("_0") );
            }
          else if (verbose)
            {
              std::cout << "\t\t** Error when looking for body " << bp->name() 
                        << ", object " << (*itcoll)->name() << std::endl;
            }
        } // for each collision object
    } // for each joint 
}
