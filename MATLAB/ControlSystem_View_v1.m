%% Init
close(findall(0, 'type', 'figure'))


window = uifigure("Position",[50 75 1500 900]);
ax_vel = uiaxes(window,"Position",[20 50 1200 400]);
ax_angle = uiaxes(window,"Position",[20 450 1200 400]);
file_dropdown = uidropdown(window,Position=[1250 790 100 20]);







%% GUI
