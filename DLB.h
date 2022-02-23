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

#ifndef _DLB_H_
#define _DLB_H_ 1

//
// Mapping the rigid transformation to the unit dual quaternion.
//
// [out] q: The unit dual quaternion of which the q[0] is the real part and the q[1] is the dual part.
//
// [in]  r: The unit quaternion which represents the rotation transform of the rigid transformation.
//
// [in]  t: The 3D vector which represnets the translation transform of the rigid transformation.
//
void unit_dual_quaternion_from_rigid_transform(DirectX::XMFLOAT4 q[2], DirectX::XMFLOAT4 const &r, DirectX::XMFLOAT3 const &t)
{
	// \hat{\boldsymbol{r}} = \boldsymbol{r_0} + [0, \overrightarrow{0}] ϵ
	// \hat{\boldsymbol{t}} = [1,  \overrightarrow{0}] + [0, 0.5 \overrightarrow{t}] ϵ
	// \hat{\boldsymbol{q}} = \hat{\boldsymbol{t}} \hat{\boldsymbol{r}}

	DirectX::XMFLOAT4 q_0 = r;

	DirectX::XMFLOAT4 q_e;
#if 1
	// \hat{\boldsymbol{t}} \hat{\boldsymbol{r}} = \boldsymbol{r_0} + [0, 0.5 \overrightarrow{t}] \boldsymbol{r_0} ϵ

	DirectX::XMFLOAT4 t_q = DirectX::XMFLOAT4(0.5F * t.x, 0.5F * t.y, 0.5F * t.z, 0.0F);

	// NOTE: "XMQuaternionMultiply" returns "Q2*Q1" rather than "Q1*Q2"!
	DirectX::XMStoreFloat4(&q_e, DirectX::XMQuaternionMultiply(DirectX::XMLoadFloat4(&r), DirectX::XMLoadFloat4(&t_q)));
#else
	// \boldsymbol{r_0} + [0, 0.5 \overrightarrow{t}] \boldsymbol{r_0} ϵ = \boldsymbol{r_0} + [0, 0.5 \overrightarrow{t}] [s_r,  \overrightarrow{v_r}] = [-0.5 \overrightarrow{t} \cdot \overrightarrow{v_r},  0.5 (s_r \overrightarrow{t} + \overrightarrow{t} \times \overrightarrow{v_r} )]

	float s_q_e;
	DirectX::XMFLOAT3 v_q_e;

	float s_r = r.w;
	DirectX::XMFLOAT3 v_r = DirectX::XMFLOAT3(r.x, r.y, r.z);

	DirectX::XMStoreFloat(&s_q_e, DirectX::XMVectorScale(DirectX::XMVector3Dot(DirectX::XMLoadFloat3(&t), DirectX::XMLoadFloat3(&v_r)), -0.5F));
	DirectX::XMStoreFloat3(&v_q_e, DirectX::XMVectorScale(DirectX::XMVectorAdd(DirectX::XMVectorScale(DirectX::XMLoadFloat3(&t), s_r), DirectX::XMVector3Cross(DirectX::XMLoadFloat3(&t), DirectX::XMLoadFloat3(&v_r))), 0.5F));

	q_e.w = s_q_e;
	q_e.x = v_q_e.x;
	q_e.y = v_q_e.y;
	q_e.z = v_q_e.z;
#endif

	q[0] = q_0;
	q[1] = q_e;
}

#endif