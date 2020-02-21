function y = polynomial(order, x, xlim, type)

switch type
    
    case 'legendre'
        
        xref = 2/(xlim(2)-xlim(1)) * (x - (xlim(2)+xlim(1))/2);
        y = legendre(order, xref);
        y = y(1,:);
        
    case 'hermite'
        
        xref = (x-xlim(1)) / xlim(2) / sqrt(2);
        x
        y = hermiteH(order, xref) / sqrt(2)^order
        
    otherwise
        error('Unknown polynomial type');
end

y = reshape(y,size(x));