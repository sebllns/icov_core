dirname =  "/home/sbelleno/_ICOV/sequences_for_fangping_v02/"
dirList = glob(strcat(dirname, "*.txt"));

x_min = 9999999999999999999;
x_max = -9999999999999999;
x_mean = 0;

z_ampl_min = 9999999999999999999;
z_ampl_max = -9999999999999999999;
z_ampl_mean = 0;

q_min = 99999999999999999;
q_max = -9999999999999999;
q_mean = 0;

p_sign_min = 9999999999999999999;
p_sign_max = -9999999999999999999;
p_sign_mean = 0;

x_count = 0;
y_count = 0;

file_analyse = cell(length(dirList), 5);

z_ampl_max_file = "";

max_file = length(dirList);

for f = 1:max_file
  
  
  disp(f)
  filename = dirList{f,1};
  
  [f1,f2,f3] = fileparts(filename);
  
  file_analyse{f, 1} = f2;

  disp(filename);

  fid = fopen (filename);

  fskipl (fid);

  x = [];

  for i = 1:9
    %subplot (3, 6, i);
    txt = fgetl (fid);
    a = str2double(strsplit(txt, {',', '\s'}, "delimitertype", "regularexpression"));
    %hist(a);
    if i==1
      t='X';
      x = a;
      
      file_analyse{f, 2} = min(x_min, min(x));
      file_analyse{f, 3} = max(x_max, max(x));
      file_analyse{f, 4} = mean(x);
      
      x_min = file_analyse{f, 2};
      x_max = file_analyse{f, 3};
      x_mean += file_analyse{f, 4};
      x_count += 1;

    else
      t= sprintf('Y_%d',i-1);
      %title(t);
      %subplot (3, 6, i+9);
      z = x - a;
      %hist(z);
      t= sprintf('Z_%d',i-1);
      %title(t);
      
      z_min = min(z);
      z_max = max(z);
      
      z_ampl = z_max - z_min;
      z_ampl_max = max(z_ampl_max, z_ampl);
      z_ampl_min = min(z_ampl_min, z_ampl);
      z_ampl_mean += z_ampl;
      
      if(z_ampl==z_ampl_max) 
        z_ampl_max_file = file_analyse{f, 1};
      end
      
      q = sum(z==0) / length(z);
      q_mean += q;
      q_max = max(q, q_max);
      q_min = min(q, q_min);
      
      p_sign = sum(((x+0.1).*(a+0.1)) < 0) / length(x);
      p_sign_mean += p_sign;
      p_sign_max = max(p_sign, p_sign_max);
      p_sign_min = min(p_sign, p_sign_min);
      
      y_count += 1;
      
    end
    
    %title(t);
  end

  fclose(fid);

end

x_mean /= x_count;
z_ampl_mean /= y_count;
q_mean /= y_count;
p_sign_mean /= y_count;

disp("************************************************");
disp("MIN X:");
disp(x_min);

disp("MAX X:");
disp(x_max);

disp("MEAN X:");
disp(x_mean);

disp("************************************************");
disp("MIN Z_MAX-Z_MIN");
disp(z_ampl_min);

disp("MAX Z_MAX-Z_MIN");
disp(z_ampl_max);

disp("MEAN Z_MAX-Z_MIN");
disp(z_ampl_mean);

%%[transpose(X<0), de2bi(abs(X), 'left-msb')]