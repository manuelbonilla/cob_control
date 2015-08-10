/*!
 *****************************************************************
 * \file
 *
 * \note
 *   Copyright (c) 2015 \n
 *   Fraunhofer Institute for Manufacturing Engineering
 *   and Automation (IPA) \n\n
 *
 *****************************************************************
 *
 * \note
 *   Project name: care-o-bot
 * \note
 *   ROS stack name: cob_control
 * \note
 *   ROS package name: cob_twist_controller
 *
 * \author
 *   Author: Marco Bezzon, email: Marco.Bezzon@ipa.fraunhofer.de
 *
 * \date Date of creation: May, 2015
 *
 * \brief
 *   This header contains the interface description of constraints
 *
 ****************************************************************/

#ifndef CONSTRAINT_H_
#define CONSTRAINT_H_

#include <kdl/chainfksolvervel_recursive.hpp>

#include "cob_twist_controller/cob_twist_controller_data_types.h"
#include "cob_twist_controller/constraints/constraint_base.h"
#include "cob_twist_controller/callback_data_mediator.h"
#include "cob_twist_controller/utils/moving_average.h"


/* BEGIN ConstraintParamFactory *********************************************************************************/
/// Creates constraint parameters and fills them with the values provided by CallbackDataMediator.
template
<typename T>
class ConstraintParamFactory
{
    public:
        static T createConstraintParams(const TwistControllerParams& twist_controller_params,
                                        CallbackDataMediator& data_mediator,
                                        const std::string& id = std::string())
        {
            T params(twist_controller_params, id);
            data_mediator.fill(params);
            return params;
        }

    private:
        ConstraintParamFactory()
        {}
};
/* END ConstraintParamFactory ***********************************************************************************/

/* BEGIN ConstraintsBuilder *************************************************************************************/
/// Class providing a static method to create constraints.
template <typename PRIO = uint32_t>
class ConstraintsBuilder
{
    public:
        static std::set<ConstraintBase_t> createConstraints(const TwistControllerParams& params,
                                                           KDL::ChainJntToJacSolver& jnt_to_jac_,
                                                           KDL::ChainFkSolverVel_recursive& fk_solver_vel,
                                                           CallbackDataMediator& data_mediator);

    private:
        ConstraintsBuilder() {}
        ~ConstraintsBuilder() {}
};
/* END ConstraintsBuilder ***************************************************************************************/

/* BEGIN CollisionAvoidance *************************************************************************************/
/// Class providing methods that realize a CollisionAvoidance constraint.
template <typename T_PARAMS, typename PRIO = uint32_t>
class CollisionAvoidance : public ConstraintBase<T_PARAMS, PRIO>
{
    public:

        CollisionAvoidance(PRIO prio,
                           T_PARAMS constraint_params,
                           CallbackDataMediator& cbdm,
                           KDL::ChainJntToJacSolver& jnt_to_jac,
                           KDL::ChainFkSolverVel_recursive& fk_solver_vel) :
            ConstraintBase<T_PARAMS, PRIO>(prio, constraint_params, cbdm),
            jnt_to_jac_(jnt_to_jac),
            fk_solver_vel_(fk_solver_vel),
            mvg_avg_dist_vec_(3, false),
            mvg_avg_distances_(3, false),
            mvg_avg_coll_pnt_vec_(3, false)

        {
            std::deque<double> weighting;
            weighting.push_back(0.7); // exponential weighting with e^(0.699) series (3rd, 4th, 5th element in series result in 1)
            weighting.push_back(0.244);
            weighting.push_back(0.056);
            mvg_avg_dist_vec_.setWeighting(weighting);
            mvg_avg_distances_.setWeighting(weighting);
            mvg_avg_coll_pnt_vec_.setWeighting(weighting);
        }

        virtual ~CollisionAvoidance()
        {}

        virtual std::string getTaskId() const;

        virtual double getCriticalValue() const;

        virtual void calculate();

        virtual double getActivationGain() const;

        virtual double getActivationThreshold() const;

        virtual double getSelfMotionMagnitude(const Eigen::MatrixXd& particular_solution,
                                              const Eigen::MatrixXd& homogeneous_solution) const;

        virtual ConstraintTypes getType() const;

        virtual Eigen::MatrixXd getTaskJacobian() const;

        virtual Eigen::VectorXd getTaskDerivatives() const;

        virtual Task_t createTask();


    private:
        double calcValue();
        double calcDerivativeValue();
        Eigen::VectorXd calcPartialValues();
        double predictValue();
        double getActivationThresholdWithBuffer() const;

        KDL::ChainJntToJacSolver& jnt_to_jac_;
        KDL::ChainFkSolverVel_recursive& fk_solver_vel_;

        MovingAverage<Eigen::Vector3d> mvg_avg_dist_vec_;
        MovingAverage<Eigen::Vector3d> mvg_avg_coll_pnt_vec_;
        MovingAvg_double_t mvg_avg_distances_;
};
/* END CollisionAvoidance ***************************************************************************************/

/* BEGIN JointLimitAvoidance ************************************************************************************/
/// Class providing methods that realize a JointLimitAvoidance constraint.
template <typename T_PARAMS, typename PRIO = uint32_t>
class JointLimitAvoidance : public ConstraintBase<T_PARAMS, PRIO>
{
    public:

        JointLimitAvoidance(PRIO prio,
                            T_PARAMS constraint_params,
                            CallbackDataMediator& cbdm)
            : ConstraintBase<T_PARAMS, PRIO>(prio, constraint_params, cbdm),
              abs_delta_max_(std::numeric_limits<double>::max()),
              abs_delta_min_(std::numeric_limits<double>::max()),  // max. delta away from min
              rel_max_(1.0), // 100% rel. range to max limit
              rel_min_(1.0) // 100% rel. range to min limit
        {}

        virtual ~JointLimitAvoidance()
        {}

        virtual std::string getTaskId() const;
        virtual void calculate();
        virtual double getActivationGain() const;
        virtual double getActivationThreshold() const;
        virtual double getSelfMotionMagnitude(const Eigen::MatrixXd& particular_solution, const Eigen::MatrixXd& homogeneous_solution) const;
        virtual ConstraintTypes getType() const;
        virtual Eigen::MatrixXd getTaskJacobian() const;
        virtual Eigen::VectorXd getTaskDerivatives() const;
        virtual Task_t createTask();


    private:
        double calcValue();
        double calcDerivativeValue();
        Eigen::VectorXd calcPartialValues();

        double abs_delta_max_;
        double abs_delta_min_;
        double rel_max_;
        double rel_min_;

};
/* END JointLimitAvoidance **************************************************************************************/

/* BEGIN JointLimitAvoidanceMid *********************************************************************************/
/// Class providing methods that realize a CollisionAvoidance constraint.
template <typename T_PARAMS, typename PRIO = uint32_t>
class JointLimitAvoidanceMid : public ConstraintBase<T_PARAMS, PRIO>
{
    public:

        JointLimitAvoidanceMid(PRIO prio,
                               T_PARAMS constraint_params,
                               CallbackDataMediator& cbdm)
            : ConstraintBase<T_PARAMS, PRIO>(prio, constraint_params, cbdm)
        {}

        virtual ~JointLimitAvoidanceMid()
        {}

        virtual std::string getTaskId() const;
        virtual void calculate();
        virtual double getActivationGain() const;
        virtual double getActivationThreshold() const;
        virtual double getSelfMotionMagnitude(const Eigen::MatrixXd& particular_solution, const Eigen::MatrixXd& homogeneous_solution) const;
        virtual ConstraintTypes getType() const;

    private:
        double calcValue();
        double calcDerivativeValue();
        Eigen::VectorXd calcPartialValues();
};
/* END JointLimitAvoidanceMid ***********************************************************************************/

/* BEGIN JointLimitAvoidanceIneq ************************************************************************************/
/// Class providing methods that realize a JointLimitAvoidance constraint based on inequalities.
template <typename T_PARAMS, typename PRIO = uint32_t>
class JointLimitAvoidanceIneq : public ConstraintBase<T_PARAMS, PRIO>
{
    public:
        JointLimitAvoidanceIneq(PRIO prio,
                            T_PARAMS constraint_params,
                            CallbackDataMediator& cbdm)
            : ConstraintBase<T_PARAMS, PRIO>(prio, constraint_params, cbdm),
              abs_delta_max_(std::numeric_limits<double>::max()),
              abs_delta_min_(std::numeric_limits<double>::max()),
              rel_max_(1.0),
              rel_min_(1.0)
        {
        }

        virtual ~JointLimitAvoidanceIneq()
        {}

        virtual std::string getTaskId() const;
        virtual void calculate();
        virtual double getActivationGain() const;
        virtual double getActivationThreshold() const;
        virtual double getSelfMotionMagnitude(const Eigen::MatrixXd& particular_solution, const Eigen::MatrixXd& homogeneous_solution) const;
        virtual ConstraintTypes getType() const;
        virtual Eigen::MatrixXd getTaskJacobian() const;
        virtual Eigen::VectorXd getTaskDerivatives() const;
        virtual Task_t createTask();

    private:
        double calcValue();
        double calcDerivativeValue();
        Eigen::VectorXd calcPartialValues();

        double abs_delta_max_;
        double abs_delta_min_;
        double rel_max_;
        double rel_min_;
};
/* END JointLimitAvoidanceIneq **************************************************************************************/

typedef ConstraintsBuilder<uint32_t> ConstraintsBuilder_t;

#include "cob_twist_controller/constraints/constraint_impl.h" // implementation of templated class

#endif /* CONSTRAINT_H_ */
