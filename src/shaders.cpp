#include <cmath>

#include "render.h"
#include "file.h"

v2_i get_tex_indicies(const v2& uv, const mesh& mesh)
{
    return {
        static_cast<int>(uv.x * static_cast<float>(mesh.diffuse.width - 1)),
        static_cast<int>(uv.y * static_cast<float>(mesh.diffuse.height - 1))
    };
}

struct blinn_shader_normal_map final : public shader{
    m4 model_view_proj{};
    m3 normal_mat{};

    //outputs for fragment
    v3 ndc_vertex[3]{};
    v2 vertex_uv[3]{};

    const char* name() override { return "Blinn Normal Map"; }

    void begin_pass() override
    {
        normal_mat = (m4_to_m3(renderer_state->projection * renderer_state->model_view)).invert().transpose();
        
        model_view_proj = renderer_state->projection * renderer_state->model_view;
    }

    v4 vertex(v3 & vertex, int face_no, int vert_no) override
    {
        const auto ret = model_view_proj  * project_4d(vertex);

        if(mesh_to_draw -> allow_lighting && mesh_to_draw->has_normal_map){
            ndc_vertex[vert_no] = project_3d(renderer_state->projection * renderer_state->model_view * project_4d(vertex));
            vertex_uv[vert_no] = mesh_to_draw->uvs[mesh_to_draw->faces[face_no].uv.e[vert_no]];
        }

        return ret;
    }



    bool fragment(const v3& bar, rgba & col, v3 interpolated_normal, v2 interpolated_uv, const v2_i& screen_pos) override
    {
        const auto tex_indicies = get_tex_indicies(interpolated_uv, *mesh_to_draw);
        
        auto dif = get_pixel(mesh_to_draw->diffuse, tex_indicies.x, tex_indicies.y);
        col = dif;

        //skip lighting calculations
        if(!mesh_to_draw->allow_lighting) return true;

        v3 normal{};

        //sample the normal map if we have one
        if (mesh_to_draw->has_normal_map) {
            interpolated_normal = normal_mat * interpolated_normal;

            //calculate tangent and bitangent for pixel 
            auto ai = m3{ ndc_vertex[1] - ndc_vertex[0], ndc_vertex[2] - ndc_vertex[0], interpolated_normal }.invert();

            v3 u_diff{ vertex_uv[1].x - vertex_uv[0].x, vertex_uv[2].x - vertex_uv[0].x, 0 };
            auto i = (ai * u_diff).normalise();

            v3 v_diff{ vertex_uv[1].y - vertex_uv[0].y, vertex_uv[2].y - vertex_uv[0].y, 0 };
            auto j = (ai * v_diff).normalise();

            auto b = m3{ i, j, interpolated_normal }.transpose();

            normal = get_normal(mesh_to_draw->normal, tex_indicies.x, tex_indicies.y);

            normal = (b * normal).normalise();
        }
        //otherwise use the passed normal
        else
        {
            normal = normal_mat * interpolated_normal;
        }
        
        //lighting
        auto l = renderer_state->light_dir;

        auto diffuse = normal.inner( l );
        if (diffuse < 0) diffuse = 0;

        float spec = 0;
        if(mesh_to_draw->has_specular_map)
        {
            auto spec_rgb = get_pixel(mesh_to_draw->spec, tex_indicies.x, tex_indicies.y);

            auto r = (normal * (normal.inner(l)) * 2 - l).normalise();
            if (r.z < 0) {
                r.z = 0;
            }
            
            spec = pow(r.z, 5 + spec_rgb.b);
        }
        
        col = col * (1.2f * diffuse + 0.6f * spec);

        //add in ambient color
        col = col + dif * 0.15f;

        return true;
    }
};