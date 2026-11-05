function ret = get_angle(vec)
    
    angle = atan2(vec(2),vec1(1));

end


clc; clear;
ANGLE_STEP = 5; % Degrees

for i = 1:10
    [x,y,z] = sph2cart(deg2rad(ANGLE_STEP * i) , rand(1), 1);
    
    fprintf("%d degrees. Vector [%.4f %.4f %.4f]. Estimated: %.2f\n", ...
        i * ANGLE_STEP, [x,y,z], rad2deg(atan2(y,x)));

end