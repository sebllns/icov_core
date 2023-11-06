import matplotlib.pyplot as plt 
import csv 
  
x = [] 
y = [] 
  
with open('erp.csv','r') as csvfile: 
    lines = csv.reader(csvfile, delimiter=',') 
    for row in lines: 
        x.append(float(row[0])) 
        y.append(float(row[1])) 
  
plt.plot(x, y, color = 'g', linestyle = 'dashed', 
         marker = 'o',label = "ERP RD") 

x = [] 
y = [] 

with open('display.csv','r') as csvfile: 
    lines = csv.reader(csvfile, delimiter=',') 
    for row in lines: 
        x.append(float(row[0])) 
        y.append(float(row[1])) 
  
plt.plot(x, y, color = 'b', linestyle = 'dashed', 
         marker = 'x',label = "DISPLAY RD") 
  
plt.xticks(rotation = 25) 
plt.xlabel('Bits per decoded pixel') 
plt.ylabel('PSNR') 
plt.title('ICOV RD', fontsize = 20) 
plt.grid() 
plt.legend() 
plt.show() 
