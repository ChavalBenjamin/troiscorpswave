#pragma once

#include <cmath>
#include <algorithm>

// ============================================================================
// ThreeBodyEngine
//
// Simule 3 corps en interaction gravitationnelle mutuelle (probleme a 3
// corps, chaotique) dans un plan 2D borne (rebond elastique sur les bords).
//
// Usage prevu : ResetToTriangle() pour placer les corps, puis
// CaptureBody1X() pour simuler une fenetre de temps et enregistrer la
// position X du corps 1 dans un buffer (destine a devenir une table d'onde).
// ============================================================================

class ThreeBodyEngine
{
public:
  struct Body
  {
    double x = 0.0, y = 0.0;
    double vx = 0.0, vy = 0.0;
    double mass = 1.0;
  };

  void SetMasses(double m1, double m2, double m3)
  {
    mBodies[0].mass = std::max(0.01, m1);
    mBodies[1].mass = std::max(0.01, m2);
    mBodies[2].mass = std::max(0.01, m3);
  }

  void SetBoxSize(double size) { mBoxSize = std::max(0.5, size); }

  // Place les 3 corps a des rayons ET des angles independants (permet par
  // exemple un corps proche du centre, un moyen, un loin - configuration
  // hierarchique - ET de choisir leur disposition angulaire plutot que le
  // triangle regulier par defaut), avec une vitesse tangentielle
  // proportionnelle a orbitalVelocity : a 0, les corps demarrent immobiles
  // (effondrement chaotique) ; en augmentant, on se rapproche d'un
  // mouvement orbital regulier (rotation rigide).
  void ResetBodies(double r1, double r2, double r3,
                    double angle1Deg, double angle2Deg, double angle3Deg,
                    double orbitalVelocity)
  {
    double radii[3] = { r1, r2, r3 };
    double angles[3] = { angle1Deg * kDegToRad, angle2Deg * kDegToRad, angle3Deg * kDegToRad };
    for (int i = 0; i < 3; i++)
    {
      mBodies[i].x = radii[i] * std::cos(angles[i]);
      mBodies[i].y = radii[i] * std::sin(angles[i]);
      // vitesse tangentielle = rotation rigide : v = omega * (-y, x)
      mBodies[i].vx = -orbitalVelocity * mBodies[i].y;
      mBodies[i].vy = orbitalVelocity * mBodies[i].x;
    }
  }

  // Simule "durationSeconds" a une resolution interne fine, et enregistre
  // la position X ET Y des 3 corps SIMULTANEMENT (un seul passage de
  // simulation). X sert a l'audio (table d'onde) ET aux LFO/CC ; Y ne sert
  // qu'a l'animation visuelle 2D (position verticale des corps).
  int CaptureAllBodiesXY(double durationSeconds,
                          float* outX1, float* outY1,
                          float* outX2, float* outY2,
                          float* outX3, float* outY3,
                          int maxSamples)
  {
    constexpr double kDt = 1.0 / 8000.0;
    int nSteps = std::min((int)(durationSeconds / kDt), maxSamples);

    for (int i = 0; i < nSteps; i++)
    {
      Step(kDt);
      outX1[i] = (float)mBodies[0].x; outY1[i] = (float)mBodies[0].y;
      outX2[i] = (float)mBodies[1].x; outY2[i] = (float)mBodies[1].y;
      outX3[i] = (float)mBodies[2].x; outY3[i] = (float)mBodies[2].y;
    }
    return nSteps;
  }

private:
  static constexpr double kPi = 3.14159265358979323846;
  static constexpr double kDegToRad = kPi / 180.0;
  static constexpr double kG = 1.0;          // constante gravitationnelle interne
  static constexpr double kSoftening = 0.05; // evite les forces infinies en cas de quasi-collision

  Body mBodies[3];
  double mBoxSize = 4.0;

  void ComputeAccelerations(double ax[3], double ay[3]) const
  {
    for (int i = 0; i < 3; i++) { ax[i] = 0.0; ay[i] = 0.0; }

    for (int i = 0; i < 3; i++)
    {
      for (int j = 0; j < 3; j++)
      {
        if (i == j) continue;
        double dx = mBodies[j].x - mBodies[i].x;
        double dy = mBodies[j].y - mBodies[i].y;
        double distSq = dx * dx + dy * dy + kSoftening * kSoftening;
        double dist = std::sqrt(distSq);
        double force = kG * mBodies[j].mass / (distSq * dist);
        ax[i] += force * dx;
        ay[i] += force * dy;
      }
    }
  }

  void Step(double dt)
  {
    double x0[3], y0[3], vx0[3], vy0[3];
    for (int i = 0; i < 3; i++) { x0[i] = mBodies[i].x; y0[i] = mBodies[i].y; vx0[i] = mBodies[i].vx; vy0[i] = mBodies[i].vy; }

    double ax1[3], ay1[3]; ComputeAccelerations(ax1, ay1);
    double k1x[3], k1y[3], k1vx[3], k1vy[3];
    for (int i = 0; i < 3; i++) { k1x[i] = vx0[i]; k1y[i] = vy0[i]; k1vx[i] = ax1[i]; k1vy[i] = ay1[i]; }

    for (int i = 0; i < 3; i++) { mBodies[i].x = x0[i] + k1x[i] * dt * 0.5; mBodies[i].y = y0[i] + k1y[i] * dt * 0.5; }
    double ax2[3], ay2[3]; ComputeAccelerations(ax2, ay2);
    double k2x[3], k2y[3], k2vx[3], k2vy[3];
    for (int i = 0; i < 3; i++) { k2x[i] = vx0[i] + k1vx[i] * dt * 0.5; k2y[i] = vy0[i] + k1vy[i] * dt * 0.5; k2vx[i] = ax2[i]; k2vy[i] = ay2[i]; }

    for (int i = 0; i < 3; i++) { mBodies[i].x = x0[i] + k2x[i] * dt * 0.5; mBodies[i].y = y0[i] + k2y[i] * dt * 0.5; }
    double ax3[3], ay3[3]; ComputeAccelerations(ax3, ay3);
    double k3x[3], k3y[3], k3vx[3], k3vy[3];
    for (int i = 0; i < 3; i++) { k3x[i] = vx0[i] + k2vx[i] * dt * 0.5; k3y[i] = vy0[i] + k2vy[i] * dt * 0.5; k3vx[i] = ax3[i]; k3vy[i] = ay3[i]; }

    for (int i = 0; i < 3; i++) { mBodies[i].x = x0[i] + k3x[i] * dt; mBodies[i].y = y0[i] + k3y[i] * dt; }
    double ax4[3], ay4[3]; ComputeAccelerations(ax4, ay4);
    double k4x[3], k4y[3], k4vx[3], k4vy[3];
    for (int i = 0; i < 3; i++) { k4x[i] = vx0[i] + k3vx[i] * dt; k4y[i] = vy0[i] + k3vy[i] * dt; k4vx[i] = ax4[i]; k4vy[i] = ay4[i]; }

    for (int i = 0; i < 3; i++)
    {
      mBodies[i].x = x0[i] + (dt / 6.0) * (k1x[i] + 2 * k2x[i] + 2 * k3x[i] + k4x[i]);
      mBodies[i].y = y0[i] + (dt / 6.0) * (k1y[i] + 2 * k2y[i] + 2 * k3y[i] + k4y[i]);
      mBodies[i].vx = vx0[i] + (dt / 6.0) * (k1vx[i] + 2 * k2vx[i] + 2 * k3vx[i] + k4vx[i]);
      mBodies[i].vy = vy0[i] + (dt / 6.0) * (k1vy[i] + 2 * k2vy[i] + 2 * k3vy[i] + k4vy[i]);

      // Rebond elastique sur les murs d'une boite carree centree sur l'origine
      if (mBodies[i].x > mBoxSize)  { mBodies[i].x = mBoxSize;  mBodies[i].vx = -mBodies[i].vx; }
      if (mBodies[i].x < -mBoxSize) { mBodies[i].x = -mBoxSize; mBodies[i].vx = -mBodies[i].vx; }
      if (mBodies[i].y > mBoxSize)  { mBodies[i].y = mBoxSize;  mBodies[i].vy = -mBodies[i].vy; }
      if (mBodies[i].y < -mBoxSize) { mBodies[i].y = -mBoxSize; mBodies[i].vy = -mBodies[i].vy; }
    }
  }
};
