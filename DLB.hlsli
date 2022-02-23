//
// Copyright (C) YuqiaoZhang(HanetakaChou)
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//

#ifndef _DLB_HLSLI_
#define _DLB_HLSLI_ 1

//
// DLB (Dual quaternion Linear Blending)
//
float2x4 dual_quaternion_linear_blending(in float2x4 q_indices_x, in float2x4 q_indices_y, in float2x4 q_indices_z, in float2x4 q_indices_w, in float4 weights)
{
#if 1
	// NOTE:
	// The original DLB does NOT check the sign of the inner product of the unit quaternion q_x_0 and q_y_0(q_z_0 q_w_0).
	// However, since the unit quaternion q and -q represent the same rotation transform, we may get the result of which the real part is zero.
	float2x4 b = weights.x * q_indices_x + weights.y * sign(dot(q_indices_x[0], q_indices_y[0])) * q_indices_y + weights.z * sign(dot(q_indices_x[0], q_indices_z[0])) * q_indices_z + weights.w * sign(dot(q_indices_x[0], q_indices_w[0])) * q_indices_w;
#else
	float2x4 b = weights.x * q_indices_x + weights.y * q_indices_y + weights.z * q_indices_z + weights.w * q_indices_w;
#endif

	// "4 Implementation Notes" of [Ladislav Kavan, Steven Collins, Jiri Zara, Carol O'Sullivan. "Geometric Skinning with Approximate Dual Quaternion Blending." SIGGRAPH 2008.](http://www.cs.utah.edu/~ladislav/kavan08geometric/kavan08geometric.html)
	// We do NOT need to calculate the exact dual part "\boldsymbol{q_\epsilon}", since the scalar part of the "\boldsymbol{q_\epsilon} {\boldsymbol{q_0}}^*" is ignored when we calculate the "\frac{1}{2}\overrightarrow{t}".
	return (b / length(b[0]));
}

//
// Mapping the rigid transformation to the unit dual quaternion.
//
// [in]  q: The unit dual quaternion of which the q[0] is the real part and the q[1] is the dual part.
//
// [out] r: The unit quaternion which represents the rotation transform of the rigid transformation.
//
// [out] t: The 3D vector which represnets the translation transform of the rigid transformation.
//
void unit_dual_quaternion_to_rigid_transform(in float2x4 q, out float4 r, out float3 t)
{
	float4 q_0 = q[0];
	float4 q_e = q[1];

	// \boldsymbol{r_0} = \boldsymbol{q_0}
	r = q_0;

	// \overrightarrow{t} = 2 (- s_ϵ \overrightarrow{v_0} + s_0 \overrightarrow{v_ϵ} - \overrightarrow{v_ϵ} \times \overrightarrow{v_0}
	// t = 2.0 * (- q_e.w * q_0.xyz + q_0.w * q_e.xyz - cross(q_e.xyz, q_0.xyz));
	t = 2.0 * (q_0.w * q_e.xyz - q_e.w * q_0.xyz + cross(q_0.xyz, q_e.xyz));
}

//
// Mapping the unit quaternion to the rotation transformation.
//
// [in]    q: The unit quaternion.
//
// [out]   p: The 3D vector which represents the position before the rotation transform.
//
// [return] : The 3D vector which represents the position after the rotation transform.
//
float3 unit_quaternion_to_rotation_transform(in float4 r, in float3 p)
{
	// "Fig. 6.7" and "Fig. 6.8" of [Quaternions for Computer Graphics](https://link.springer.com/book/10.1007/978-1-4471-7509-4)
	// "Lemma 4" of [Ladislav Kavan, Steven Collins, Jiri Zara, Carol O'Sullivan. "Geometric Skinning with Approximate Dual Quaternion Blending." SIGGRAPH 2008.](http://www.cs.utah.edu/~ladislav/kavan08geometric/kavan08geometric.html)

	return (p + 2.0 * cross(r.xyz, cross(r.xyz, p) + r.w * p));
}

#endif