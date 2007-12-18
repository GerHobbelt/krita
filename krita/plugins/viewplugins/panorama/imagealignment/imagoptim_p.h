/*
 * panoptim_p.h -- Part of Krita
 *
 * Copyright (c) 2007 Cyrille Berger (cberger@cberger.net)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation; version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA. */

#ifndef _PANOPTIM_P_H_
#define _PANOPTIM_P_H_

#include "optimization.h"
#include "imagoptim_functions.h"
#include "kis_control_point.h"
#include "kis_control_points.h"
#include "kis_image_alignment_model_p.h"
#include "matching.h"

// For each image there is four parameters to estimate : translation (2 parameters) + distortion (2 parameters)
template<class _TFunction_>
class PanoptimFunction : public KisImageAlignmentModel::OptimizationFunction {
  public:
    PanoptimFunction(const KisControlPoints& cps, double xc, double yc, int width, int height) : m_xc(xc), m_yc(yc), m_norm(4.0 / ( width * width + height * height ) ), m_epsilon(1e-3)
    {
        foreach(KisControlPoint cp, cps.controlPoints())
        {
            for(QList<int>::iterator it = cp.frames.begin(); it != cp.frames.end(); ++it)
            {
                QList<int>::iterator it2 = it;
                ++it2;
                for(; it2 != cp.frames.end(); ++it2)
                {
                    int frameRef = *it;
                    int frameMatch = *it2;
                    QPointF ref = cp.positions[frameRef];
                    QPointF match = cp.positions[frameMatch];
                    kDebug() << ref << " = " << frameRef << " ========= " << frameMatch << " = " << match;
                    m_functions.push_back(_TFunction_( m_xc, m_yc, m_norm, frameRef, ref.x(), ref.y(), frameMatch, match.x(), match.y() ) );
                }
            }
            
/*            if( cp.frames.contains(0) and cp.frames.contains(1) )
            {
                QPointF ref = cp.positions[0];
                QPointF match = cp.positions[1];
                kDebug() << ref << " =========== " << match;
                m_functions.push_back(_TFunction_( m_xc, m_yc, m_norm, 0, ref.x(), ref.y(), 1, match.x(), match.y() ) );
            }*/
        }
        kDebug() << "Nb of functions is " << m_functions.size();
#if 0
      for(lMatches::const_iterator it = m_matches.begin(); it != m_matches.end(); ++it)
      {
        m_functions.push_back(_TFunction_(indx, m_xc, m_yc, m_norm, it->ref->x(), it->ref->y(), it->match->x(), it->match->y()));
      }
#endif
    }
  public:
    std::vector<double> values(const std::vector<double>& parameters)
    {
      std::vector<double> v;
      // Compute the values
      for(typename std::vector<_TFunction_>::iterator it = m_functions.begin(); it != m_functions.end(); ++it)
      {
        double f1,f2;
        it->f(parameters, f1, f2);
        v.push_back(f1);
        v.push_back(f2);
//         kDebug(41006) << f1 <<" = f1 f2 =" << f2 <<"" << it->m_i1 <<"" << it->m_j1 <<"" << it->m_i2 <<"" << it->m_j2;
      }
      return v;
    }
    double pow2(double f) { return f*f; }
    gmm::row_matrix< gmm::rsvector<double> > jacobian(const std::vector<double>& parameters)
    {
      gmm::row_matrix< gmm::wsvector<double> > jt(count(), parameters.size());
      
      // Compute the jacobian
      int pos = 0;
      for(typename std::vector<_TFunction_>::iterator it = m_functions.begin(); it != m_functions.end(); ++it)
      {
        it->jac(parameters, jt, pos);
        pos += 2;
      }
      // Copy the result to a read matrix
      gmm::row_matrix< gmm::rsvector<double> > jr(count(),parameters.size());
      gmm::copy(jt,jr);
      return jr;
    }
    inline int count()
    {
      return 2 * m_functions.size();
    }
  private:
    std::vector<_TFunction_> m_functions;
    double m_xc, m_yc, m_norm;
    const double m_epsilon;
};

#endif
