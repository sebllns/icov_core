dirname =  "/home/sbelleno/_ICOV/sequences_for_fangping_v02/"
dirList = glob(strcat(dirname, "*.txt"));

for f = 1:length(dirList)
  
filename = dirList{f,1};

disp(filename);

fid = fopen (filename);

fskipl (fid);

x = [];

z_min = [];
z_max = [];

for i = 1:9
  subplot (3, 6, i);
  txt = fgetl (fid);
  a = str2double(strsplit(txt, {',', '\s'}, "delimitertype", "regularexpression"));
  hist(a);
  if i==1
    t='X';
    x = a;
  else
    t= sprintf('Y_%d',i-1);
    title(t);
    subplot (3, 6, i+9);
    z = x - a;
    hist(z);
    t= sprintf('Z_%d',i-1);
    title(t);
    
    z_min = [z_min min(z)];
    z_max = [z_max max(z)];
  end
  
  title(t);
end

fclose(fid);

end

disp("MIN X:");
disp(min(x));

disp("MAX X:");
disp(max(x));

disp("MIN Z_MAX-Z_MIN");

