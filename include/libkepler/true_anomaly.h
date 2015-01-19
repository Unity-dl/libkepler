#ifndef LIBKEPLER_TRUE_ANOMALY_H
#define LIBKEPLER_TRUE_ANOMALY_H

double true_radius(double p, double e, double f);
double true_anomaly_from_radius(double p, double e, double r);

double true_dfdt(double mu, double p, double e, double f);

double true_velocity(double mu, double p, double e, double f);
double true_velocity_radial(double mu, double p, double e, double f);
double true_velocity_horizontal(double mu, double p, double e, double f);

double true_tan_phi(double e, double f);
double true_flight_path_angle(double e, double f);

double true_x(double p, double e, double f);
double true_y(double p, double e, double f);
double true_xdot(double mu, double p, double e, double f);
double true_ydot(double mu, double p, double e, double f);

#endif