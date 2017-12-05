\ Model second
\ LP format - for model browsing. Use MPS format to capture full model detail.
Minimize
  - 0.233 x0 - 3.422 x1 - 0.1904 x2 - 0.5411 x3 - 0.045 x4 - 1.271 x5
   - 0.955 x6 + [ 20 x0 ^2 + 40 x1 ^2 + 60 x2 ^2 + 80 x3 ^2 + 100 x4 ^2
   + 120 x5 ^2 + 140 x6 ^2 + 200 y0 ^2 + 0.4 y0 * y1 + 400 y1 ^2 ] / 2 
Subject To
 first_constraint: 1.508 x0 + 0.7802 x1 + 1.8796 x2 + 4.256 x3 + 1.335 x4
   + 2.026 x5 + 1.909 x6 - y0 = 0
 second_constraint: 4.228 x0 + 1.2945 x1 + 0.827 x2 + 2.149 x3 + 2.353 x4
   + 0.3026 x5 + 1.487 x6 - y1 = 0
 convexity: x0 + x1 + x2 + x3 + x4 + x5 + x6 = 1
Bounds
 x0 <= 0.6
 x1 <= 0.8
 x2 <= 0.8
 x3 <= 0.5
 x4 <= 0.5
 x5 <= 0.26
 x6 <= 0.99
 -100 <= y0 <= 100
 -100 <= y1 <= 100
End
