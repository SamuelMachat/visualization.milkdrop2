/*
*  Copyright � 2010-2015 Team Kodi
*  http://kodi.tv
*
*  This program is free software: you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation, either version 2 of the License, or
*  (at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*/

struct PS_INPUT
{
  float4 Diffuse   : COLOR;
  float4 Tex0      : TEXCOORD0;
  float2 Tex1      : TEXCOORD1;
};

float4 main(PS_INPUT In) : SV_TARGET
{
  return In.Diffuse;
}