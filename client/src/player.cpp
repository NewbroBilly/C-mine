#include <player.h>
#include <movement.h>
#include <avatar.h>

Player::Player(const dvec3& _pos)
{
    pos = _pos;
    vel = dvec3(0);

    grounded = false;

    cam = Camera(vec3(0, 1.62, 0), 80, win_size.x / (float)win_size.y, 0.1f, 1000.0f);

    hitbox = Hitbox(dvec3(-0.3, 0, -0.3), dvec3(0.6, 1.8, 0.6));

    hit_info.hit = false;

    inventory.set_item(0, 0, STONE);
    inventory.set_item(0, 1, DIRT);
    inventory.set_item(0, 2, GRASS_BLOCK);
    inventory.set_item(0, 3, GRASS);
    inventory.set_item(0, 4, LEAVES);
    inventory.set_item(0, 5, LOG);
    inventory.set_item(0, 6, PLANKS);
    inventory.set_item(0, 7, GLASS);
    inventory.set_item(0, 8, TORCH);
    int items[] = {STONE, DIRT, GRASS_BLOCK, GRASS, LEAVES, LOG, PLANKS, GLASS, TORCH};
    for (int i=0;i<9;++i) hotbar.set_item(i, items[i]);
}

vec3 get_input()
{
    vec3 input = vec3(0);

    if (is_key_pressed(SDLK_w))         input += vec3(0, 0, 1);
    if (is_key_pressed(SDLK_s))         input += vec3(0, 0, -1);
    if (is_key_pressed(SDLK_a))         input += vec3(-1, 0, 0);
    if (is_key_pressed(SDLK_d))         input += vec3(1, 0, 0);
    if (is_key_pressed(SDLK_LCTRL))     input += vec3(0, -1, 0);
    if (is_key_pressed(SDLK_SPACE))     input += vec3(0, 1, 0);

    return input;
}

void Player::handle_keys()
{
    for (int i = 0; i < 9; i++)
        if (is_key_down(SDLK_1 + i))
            hotbar.set_index(i);

    if (is_key_down(SDLK_q))
        hotbar.set_item(-1);
}

void Player::handle_clicks()
{
    if (show_inventory)
    {
        if (inventory.over != NULL && is_button_down(0))
        {
            if (!inventory.over->is_empty())
                hotbar.set_item(inventory.over->get_item());
        }
    }
    else
    {
        vec3 ray_origin = (vec3)pos + cam.pos;
        vec3 ray_direction = cam.forward;

        Chunk* chunk = world->get_chunk_from_pos(ray_origin);

        hit_info = raycast(ray_origin, ray_direction, 4.5f, chunk);

        if (hit_info.hit)
        {
            if (is_button_down(0))
                world->remove_block(hit_info.chunk, hit_info.block_pos);

            if (is_button_down(1))
            {
                int id = hit_info.chunk->get(hit_info.block_pos)->id;

                int i = hotbar.find_item(id);

                if (i != -1)
                    hotbar.set_index(i);
                else
                {
                    i = hotbar.find_empty();

                    if (i != -1)
                        hotbar.set_index(i);

                    hotbar.set_item(id);
                }
            }

            if (is_button_down(2))
            {
                if (!hotbar.is_empty())
                {
                    ivec3 new_pos = hit_info.block_pos + hit_info.face_normal;

                    BlockState bs = { (char)hotbar.get_item(), 0, 0 };

                    bool failed = false;

                    if (bs.id == BlockID::LOG)
                    {
                        if (hit_info.face_normal.x != 0)
                            bs.data1 = 0;
                        else if (hit_info.face_normal.y != 0)
                            bs.data1 = 1;
                        else
                            bs.data1 = 2;
                    }
                    else if (bs.id == BlockID::TORCH)
                    {
                        if (hit_info.face_normal.y == 1)         bs.data1 = 0;
                        else if (hit_info.face_normal.z == 1)    bs.data1 = 1;
                        else if (hit_info.face_normal.z == -1)   bs.data1 = 2;
                        else if (hit_info.face_normal.x == 1)    bs.data1 = 3;
                        else if (hit_info.face_normal.x == -1)   bs.data1 = 4;
                        else                                    failed = true;
                    }

                    if (!failed)
                    {
                        Hitbox cube = Hitbox((dvec3)new_pos + chunk_pos_to_world_hp(hit_info.chunk->pos) - pos, dvec3(1));

                        if (!intersect(cube, hitbox))
                            world->place_block(hit_info.chunk, new_pos, &bs);
                    }
                }
            }
        }
    }
}

void Player::move()
{
    vec3 right = normalize(cross(cam.up, cam.forward));
    if (mouse_control && !show_inventory) {
        int x,y; SDL_GetRelativeMouseState(&x,&y);
        vec3 f = rotate(cam.forward, radians(.07f*y), right);
        if (dot(right,cross(cam.up,f)) > .01f) cam.forward = f;
        cam.forward = rotate(cam.forward, radians(.07f*x), cam.up);
    }
    vec3 input = show_inventory ? vec3(0) : get_input();
    double dt = std::min(delta_time,.05);
    if (!survival_mode) {
        vel = dvec3(0);
        if (length(input)>0) pos += dvec3(right*input.x + cam.up*input.y + cam.forward*input.z) * (is_key_pressed(SDLK_LSHIFT)?20.:7.) * dt;
        return;
    }
    dvec3 direction = dvec3(right)*double(input.x) + normalize(dvec3(cam.forward.x,0,cam.forward.z))*double(input.z);
    if (length(direction)>0) direction = normalize(direction)*(is_key_pressed(SDLK_LSHIFT)?5.6:4.3);
    auto solid = [](int x,int y,int z) {
        if (y<0) return true;
        if (y>=CHUNK_HEIGHT) return false;
        Chunk* c = world->get_chunk_from_pos(dvec3(x,y,z));
        if (!c) return true;
        ivec3 local = dvec3(x,y,z)-chunk_pos_to_world_hp(c->pos);
        int id = c->get(local)->id;
        return id != AIR && id != GRASS && id != TORCH;
    };
    jump_pending |= !show_inventory && is_key_down(SDLK_SPACE);
    accumulator += dt;
    Movement::State state{pos,vel,grounded};
    while (accumulator >= 1./120.) {
        Movement::tick(state,dvec2(direction.x,direction.z),jump_pending,1./120.,solid);
        jump_pending = false;
        accumulator -= 1./120.;
    }
    pos = state.position; vel = state.velocity; grounded = state.grounded;
    walk_phase += length(dvec2(vel.x,vel.z))*dt*3.5;
}

Camera Player::view_camera() {
    Camera view = cam;
    if (!third_person) return view;
    vec3 offset = cam.forward * (third_person == 1 ? -3.5f : 3.5f);
    float fraction = 0;
    for (float t=.05f;t<=1.f;t+=.05f) {
        dvec3 p = pos + dvec3(cam.pos + offset*t);
        Chunk* c = world->get_chunk_from_pos(p);
        if (!c || p.y<0 || p.y>=CHUNK_HEIGHT) break;
        ivec3 local = floor(p)-chunk_pos_to_world_hp(c->pos);
        int id=c->get(local)->id;
        if (id!=AIR && id!=GRASS && id!=TORCH) break;
        fraction=t;
    }
    view.pos += offset*fraction;
    if (third_person==2) view.forward = -cam.forward;
    return view;
}

void Player::update()
{
    if (!world->spawned) return;
    handle_keys();
    handle_clicks();
    move();
}

void Player::draw()
{
    Camera view = view_camera();
    mat4 cam_mat = view.matrix((vec3)pos);
    if (third_person) Avatar::draw(pos,cam.forward,walk_phase,cam_mat);
    line_shader.bind();
    line_shader.upload_mat4("cam_mat", cam_mat);

    if (hit_info.hit)
    {
        line_shader.upload_vec4("color", vec4(0, 0, 0, 0.5f));

        Hitbox block_hitbox;

        block_hitbox.pos = (dvec3)hit_info.block_pos + chunk_pos_to_world_hp(hit_info.chunk->pos) - (dvec3)cam.forward * 0.005;
        block_hitbox.size = dvec3(1);

        block_hitbox.draw();
    }

    if (show_debug)
    {
        line_shader.upload_vec4("color", vec4(0, 0, 1, 1));

        Hitbox _hitbox = hitbox;
        _hitbox.pos += pos;
        _hitbox.draw();
    }
}

void Player::destroy()
{
    hotbar.s_hotbar.destroy();
    hotbar.s_selector.destroy();
    inventory.s_inventory.destroy();
    inventory.s_highlight.destroy();
    delete this;
}
