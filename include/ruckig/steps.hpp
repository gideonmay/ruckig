#pragma once

#include <array>
#include <optional>

#include <ruckig/profile.hpp>


namespace ruckig {

//! Which times are possible for synchronization?
struct Block {
    struct Interval {
        double left, right; // [s]
        Profile profile; // Profile corresponding to right (end) time

        explicit Interval(double left, double right, const Profile& profile): left(left), right(right), profile(profile) { };
    };

    double t_min; // [s]
    Profile p_min; // Save min profile so that it doesn't need to be recalculated in Step2

    // Max. 2 intervals can be blocked: a and b with corresponding profiles, the order does not matter
    std::optional<Interval> a, b;

    explicit Block() { }
    explicit Block(const Profile& p_min): p_min(p_min), t_min(p_min.t_sum[6] + p_min.t_brake.value_or(0.0)) { }

    bool is_blocked(double t) const {
        return (t < t_min) || (a && a->left < t && t < a->right) || (b && b->left < t && t < b->right);
    }
};


//! Calculates (pre-) trajectory to get current state below the limits
class Brake {
    static constexpr double eps {2e-14};

    static void acceleration_brake(double v0, double a0, double vMax, double aMax, double jMax, std::array<double, 2>& t_brake, std::array<double, 2>& j_brake);
    static void velocity_brake(double v0, double a0, double vMax, double aMax, double jMax, std::array<double, 2>& t_brake, std::array<double, 2>& j_brake);

public:
    static void get_brake_trajectory(double v0, double a0, double vMax, double vMin, double aMax, double jMax, std::array<double, 2>& t_brake, std::array<double, 2>& j_brake);
};


class Step1 {
    using Limits = Profile::Limits;
    using Teeth = Profile::Teeth;

    double p0, v0, a0;
    double pf, vf, af;
    double vMax, vMin, aMax, jMax;

    // Pre-calculated expressions
    double pd;
    double v0_v0, vf_vf;
    double a0_a0, a0_p3, a0_p4, a0_p5, a0_p6;
    double af_af, af_p3, af_p4, af_p5, af_p6;
    double aMax_aMax;
    double jMax_jMax;

    // Max 6 valid profiles
    std::array<Profile, 6> valid_profiles;
    size_t valid_profile_counter;

    void time_up_acc0_acc1_vel(Profile& profile, double vMax, double aMax, double jMax);
    void time_up_acc1_vel(Profile& profile, double vMax, double aMax, double jMax);
    void time_up_acc0_vel(Profile& profile, double vMax, double aMax, double jMax);
    void time_up_vel(Profile& profile, double vMax, double aMax, double jMax);
    void time_up_acc0_acc1(Profile& profile, double vMax, double aMax, double jMax);
    void time_up_acc1(Profile& profile, double vMax, double aMax, double jMax);
    void time_up_acc0(Profile& profile, double vMax, double aMax, double jMax);
    void time_up_none(Profile& profile, double vMax, double aMax, double jMax);

    void time_down_acc0_acc1_vel(Profile& profile, double vMin, double aMax, double jMax);
    void time_down_acc1_vel(Profile& profile, double vMin, double aMax, double jMax);
    void time_down_acc0_vel(Profile& profile, double vMin, double aMax, double jMax);
    void time_down_vel(Profile& profile, double vMin, double aMax, double jMax);
    void time_down_acc0_acc1(Profile& profile, double vMin, double aMax, double jMax);
    void time_down_acc1(Profile& profile, double vMin, double aMax, double jMax);
    void time_down_acc0(Profile& profile, double vMin, double aMax, double jMax);
    void time_down_none(Profile& profile, double vMin, double aMax, double jMax);

    inline void add_profile(Profile profile, double jMax) {
        profile.direction = (jMax > 0) ? Profile::Direction::UP : Profile::Direction::DOWN;
        valid_profiles[valid_profile_counter] = profile;
        ++valid_profile_counter;
    }

    inline void add_interval(std::optional<Block::Interval>& interval, size_t left, size_t right, double t_brake) const {
        // if (interval) {
        //     return;
        // }

        // if (valid_profiles[left].teeth != valid_profiles[right].teeth) {
        //     return;
        // }

        // if (valid_profiles[left].direction != valid_profiles[right].direction) {
        //     return;
        // }
        
        const double left_duration = valid_profiles[left].t_sum[6] + t_brake;
        const double right_duraction = valid_profiles[right].t_sum[6] + t_brake;
        if (left_duration < right_duraction) {
            interval = Block::Interval(left_duration, right_duraction, valid_profiles[right]);
        } else {
            interval = Block::Interval(right_duraction, left_duration, valid_profiles[left]);
        }
    }

    bool calculate_block();

public:
    Block block;

    explicit Step1(double p0, double v0, double a0, double pf, double vf, double af, double vMax, double vMin, double aMax, double jMax);

    bool get_profile(const Profile& input);
};


class Step2 {
    using Limits = Profile::Limits;
    using Teeth = Profile::Teeth;

    double tf;
    double p0, v0, a0;
    double pf, vf, af;
    double vMax, vMin, aMax, jMax;

    // Pre-calculated expressions
    double pd;
    double tf_tf, tf_p3, tf_p4;
    double vd, vd_vd;
    double ad, ad_ad;
    double v0_v0, vf_vf;
    double a0_a0, a0_p3, a0_p4, a0_p5, a0_p6;
    double af_af, af_p3, af_p4, af_p5, af_p6;
    double aMax_aMax, aMax_p4;
    double jMax_jMax, jMax_p4;

    bool time_up_acc0_acc1_vel(Profile& profile, double vMax, double aMax, double jMax);
    bool time_up_acc1_vel(Profile& profile, double vMax, double aMax, double jMax);
    bool time_up_acc0_vel(Profile& profile, double vMax, double aMax, double jMax);
    bool time_up_vel(Profile& profile, double vMax, double aMax, double jMax);
    bool time_up_acc0_acc1(Profile& profile, double vMax, double aMax, double jMax);
    bool time_up_acc1(Profile& profile, double vMax, double aMax, double jMax);
    bool time_up_acc0(Profile& profile, double vMax, double aMax, double jMax);
    bool time_up_none(Profile& profile, double vMax, double aMax, double jMax);

    bool time_down_acc0_acc1_vel(Profile& profile, double vMin, double aMax, double jMax);
    bool time_down_acc1_vel(Profile& profile, double vMin, double aMax, double jMax);
    bool time_down_acc0_vel(Profile& profile, double vMin, double aMax, double jMax);
    bool time_down_vel(Profile& profile, double vMin, double aMax, double jMax);
    bool time_down_acc0_acc1(Profile& profile, double vMin, double aMax, double jMax);
    bool time_down_acc1(Profile& profile, double vMin, double aMax, double jMax);
    bool time_down_acc0(Profile& profile, double vMin, double aMax, double jMax);
    bool time_down_none(Profile& profile, double vMin, double aMax, double jMax);

public:
    explicit Step2(double tf, double p0, double v0, double a0, double pf, double vf, double af, double vMax, double vMin, double aMax, double jMax);

    bool get_profile(Profile& profile);
};

} // namespace ruckig