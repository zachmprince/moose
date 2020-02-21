clear;clc;close all;

coeff_file = 'master_1dnorm_mc_out_pc_coeff_0002.csv';
samp_file = 'master_1dnorm_mc_out_pc_samp_0002.csv';
storage_file = 'master_1dnorm_mc_out_storage_0002.csv';

M = csvread(coeff_file,1);
coeff = M(:,1);
order = M(:,2:end);
poly_type = {'hermite'};

M = csvread(samp_file, 1);
pc_value = M(:,1);
pc_samp = M(:,2:end);
xlim = [5; 0.5];

M = csvread(storage_file,1);
value = M;


% val = ones(size(pc_samp,1), size(order,1));
% for i=1:size(order,1)
%     for d=1:size(order,2)
%         val(:,i) = val(:,i).*polynomial(order(i,d), pc_samp(:,d), xlim(:,d), poly_type{d});
%     end
% end
% matlab_value = val * coeff;        

histogram(value, 10, 'Normalization', 'probability');
hold on
histogram(pc_value(pc_value<max(value)), 10, 'Normalization', 'probability');
% histogram(matlab_value, 'Normalization', 'probability');
